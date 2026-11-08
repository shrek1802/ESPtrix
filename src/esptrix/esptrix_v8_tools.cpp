// ESPTRIX V8 — hardware diagnostics, independently developed Bruce derivative.
// SPDX-License-Identifier: AGPL-3.0-or-later
// RX-only radio features. No credentials, replay, emission or fabricated decodes.
#include "esptrix_v8_tools.h"
#if defined(T_EMBED_1101)
#include "core/display.h"
#include "core/sd_functions.h"
#include "core/mykeyboard.h"
#include "core/bus_HAL.h"
#include "modules/rf/rf_utils.h"
#include "modules/rf/protocols/rf_decoder.h"
#include "modules/rfid/PN532.h"
#include "modules/others/mic.h"
#include <globals.h>
#include <WiFi.h>
#include <SD.h>
#include <NimBLEDevice.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <vector>

namespace esptrix {
namespace {
constexpr uint16_t BG=0x0000, ESPTV8_WHITE=0xffff, BLUE=0x067f, CYAN=0x07ff, DIM=0x9cf3, GRID=0x19a6, AMBER=0xfde0;
constexpr float RF_FREQS[]={315.00f,433.92f,868.35f};
constexpr unsigned MAX_EDGES=320;

void clearKeys() {
    check(EscPress); check(SelPress); check(PrevPress); check(NextPress);
#ifdef HAS_ENCODER
    drainRotarySteps();
#endif
}
void screen(const char *title, const char *sub="") {
    tft.setTextFont(1); tft.setTextSize(1); tft.setTextDatum(TL_DATUM);
    tft.fillScreen(BG); tft.setTextColor(CYAN,BG);
    tft.drawString(title,8,5,1);
    if (*sub) {tft.setTextColor(DIM,BG); tft.drawRightString(sub,tftWidth-8,6,1);}
    tft.drawFastHLine(8,22,tftWidth-16,GRID);
    tft.setTextColor(DIM,BG); tft.drawString("BACK: exit",8,tftHeight-12,1);
}
void line(int y,const String &label,const String &value,uint16_t color=ESPTV8_WHITE) {
    tft.fillRect(8,y,tftWidth-16,15,BG);
    tft.setTextColor(DIM,BG); tft.drawString(label,9,y+2,1);
    tft.setTextColor(color,BG); tft.drawRightString(value,tftWidth-10,y+2,1);
}
void info(const String &s) { displayInfo(s,true); }
void menu(const char *title, std::vector<Option> items) {
    items.push_back({"Back",[](){}});
    for (;;) { int i=loopOptions(items,MENU_TYPE_SUBMENU,title); if(i<0||i==static_cast<int>(items.size())-1) break; }
}
void hold() { clearKeys(); while(!check(EscPress)&&!check(SelPress)) delay(25); clearKeys(); }
String fmhz(float f) {return String(f,2)+" MHz";}
void noRf(){displayError("Select/attach CC1101",true);}
bool rfPresent(){if(bruceConfigPins.rfModule!=CC1101_SPI_MODULE){noRf();return false;}return true;}
bool saveText(const String &path,const String &payload) {
    if(!setupSdCard()){displayError("SD card unavailable",true);return false;}
    File file=SD.open(path.c_str(),FILE_APPEND);
    if(!file){displayError("Cannot open SD report",true);return false;}
    file.print(payload); file.close(); info("Saved: "+path); return true;
}
String csv(const String &s) {
    String clean=s;
    if(clean.length() && (clean[0]=='='||clean[0]=='+'||clean[0]=='-'||clean[0]=='@')) clean="'"+clean;
    clean.replace("\"","\"\""); clean.replace("\r"," ");clean.replace("\n"," ");
    return "\""+clean+"\"";
}

struct PulseCapture {
    uint16_t us[MAX_EDGES]{};
    uint8_t level[MAX_EDGES]{};
    unsigned count=0;
    float mhz=433.92f;
    int initialRssi=-120;
    uint32_t elapsedUs=0;
    bool success=false;
};
// Receive-only: use Bruce's tested RMT capture session rather than busy GPIO
// polling. Tune once and arm for ten seconds, accepting the first captured
// OOK frame. RMT measures signed HIGH/LOW pulse widths in microseconds.
// This does not decode vehicle-specific protocols or rolling codes.
void capturePulse(PulseCapture &c,float mhz) {
    c=PulseCapture{};c.mhz=mhz;
    if(!rfPresent())return;
    clearKeys();
    screen("RF SIGNAL LISTENER","OOK / RX ONLY");
    line(32,"Frequency",fmhz(mhz),CYAN);
    line(55,"Status","Starting receiver...");
    if(!initRfModule("rx",mhz)){displayError("CC1101 init failed",true);return;}
    RfRxSession receiver;
    if(!receiver.begin()){
        deinitRfModule();
        displayError("RMT receiver unavailable",true);
        return;
    }
    c.success=true;
    c.initialRssi=ELECHOUSE_cc1101.getRssi();
    int peak=c.initialRssi;
    const uint32_t start=millis();
    uint32_t lastDisplay=0;
    bool cancelled=false;
    std::vector<int> durations;
    while(static_cast<uint32_t>(millis()-start)<10000UL){
        if(check(EscPress)){cancelled=true;break;}
        if(receiver.poll(durations) && durations.size()>=12){
            c.count=0;
            for(size_t i=0;i<durations.size() && c.count<MAX_EDGES;i++){
                const int width=durations[i];
                if(width==0)continue;
                const unsigned long magnitude=static_cast<unsigned long>(std::abs(width));
                c.us[c.count]=static_cast<uint16_t>(std::min<unsigned long>(magnitude,65535UL));
                c.level[c.count]=static_cast<uint8_t>(width>0?1:0);
                ++c.count;
            }
            break;
        }
        if(lastDisplay==0 || static_cast<uint32_t>(millis()-lastDisplay)>=250UL){
            int rssi=ELECHOUSE_cc1101.getRssi();
            if(rssi>peak)peak=rssi;
            line(54,"Status","LISTENING - press remote",CYAN);
            line(78,"Live / peak RSSI",String(rssi)+" / "+String(peak)+" dBm");
            const uint32_t elapsed=millis()-start;
            line(102,"Time remaining",String((10000UL-std::min<uint32_t>(elapsed,10000UL)+999)/1000)+" s");
            line(126,"Exit","BACK cancels");
            lastDisplay=millis();
        }
        delay(8);
    }
    c.elapsedUs=static_cast<uint32_t>(millis()-start)*1000UL;
    receiver.end();
    deinitRfModule();
    clearKeys();
    if(cancelled)c.success=false;
}
void pulseGraph(const PulseCapture &c,unsigned zoom);
void showCaptured(const PulseCapture &c,unsigned zoom=1){
    if(!c.success)return;
    pulseGraph(c,zoom);
    if(c.count==0){
        tft.setTextColor(AMBER,BG);
        tft.drawString("No OOK frame in 10 sec",15,119,1);
    }
    hold();
}
unsigned long pulseSum(const PulseCapture &c){unsigned long total=0;for(unsigned i=0;i<c.count;i++)total+=c.us[i];return total;}
int pulseSimilarity(const PulseCapture &a,const PulseCapture &b) {
    if(a.count<8||b.count<8) return -1;
    unsigned pairs=std::min(a.count,b.count);
    unsigned comparable=0;
    for(unsigned i=1;i<pairs;i++) {
        unsigned hi=std::max(a.us[i],b.us[i]);
        unsigned lo=std::min(a.us[i],b.us[i]);
        if(hi>0 && (hi-lo)*100UL<=hi*25UL && a.level[i]==b.level[i]) comparable++;
    }
    // Timing-only similarity, not a key match or an authenticity verdict.
    return static_cast<int>(100UL*comparable/(pairs-1));
}
void pulseGraph(const PulseCapture &c,unsigned zoom=1) {
    screen("RF PULSE TRACE","OOK / RX");
    line(28,"Frequency",fmhz(c.mhz),CYAN);
    line(45,"Edges",String(c.count));
    line(62,"Pre-capture RSSI",String(c.initialRssi)+" dBm");
    const int x0=9,x1=tftWidth-9,y0=83,y1=137;
    tft.drawRect(x0,y0,x1-x0,y1-y0,GRID);
    if(!c.count){tft.setTextColor(AMBER,BG);tft.drawString("No demodulated edges",17,102,1);return;}
    unsigned long total=pulseSum(c);
    const unsigned long window=std::max(8000UL,total/static_cast<unsigned long>(zoom));
    int x=x0+1, lastY=c.level[0]?y0+8:y1-8;
    for(unsigned i=0;i<c.count;i++) {
        const int y=c.level[i]?y0+8:y1-8;
        int dx=std::max(1L,static_cast<long>(c.us[i])*static_cast<long>(x1-x0-3)/static_cast<long>(window));
        int nx=std::min(x1-1,x+dx);
        if(i>0)tft.drawFastVLine(x,std::min(y,lastY),std::abs(lastY-y)+1,CYAN);
        tft.drawFastHLine(x,y,std::max(1,nx-x),CYAN);
        x=nx;lastY=y;
        if(x>=x1-1)break;
    }
}
String pulseCsv(const String &name,const PulseCapture &c) {
    String out="capture,frequency_mhz,edge_index,level,duration_us\n";
    for(unsigned i=0;i<c.count;i++)out+=csv(name)+","+String(c.mhz,2)+","+String(i)+","+String(c.level[i])+","+String(c.us[i])+"\n";
    return out;
}

// Be conservative: a strong RSSI reading is not necessarily a matching protocol.
void tpmsAt(float mhz){
    if(!rfPresent())return;
    PulseCapture c;capturePulse(c,mhz);if(!c.success)return;
    pulseGraph(c);
    tft.fillRect(8,142,tftWidth-16,15,BG);
    tft.setTextColor(AMBER,BG);tft.drawString("NO VERIFIED TPMS DECODER",8,144,1);
    hold();
    menu("TPMS CAPTURE",{
        {"Save pulse timing to SD",[c](){saveText("/esptrix_v8_tpms.csv",pulseCsv("tpms",c));}},
        {"What is decoded?",[](){info("OOK activity only; sensor ID, pressure, temperature and FSK protocols are not decoded.");}}
    });
}

} // namespace

void v8KeyFobAnalyzer() {
    if(!rfPresent())return;
    float mhz=433.92f;
    PulseCapture baseline,second;
    menu("KEY FOB ANALYSER",{
        {"315 MHz - listen for fob",[&](){mhz=315.00f;capturePulse(baseline,mhz);showCaptured(baseline);}},
        {"433.92 MHz - listen for fob",[&](){mhz=433.92f;capturePulse(baseline,mhz);showCaptured(baseline);}},
        {"868.35 MHz - listen for fob",[&](){mhz=868.35f;capturePulse(baseline,mhz);showCaptured(baseline);}},
        {"Capture A (baseline)",[&](){capturePulse(baseline,mhz);showCaptured(baseline);}},
        {"Capture B (compare)",[&](){capturePulse(second,mhz);showCaptured(second);}},
        {"Compare pulse timing",[&](){
            screen("REMOTE COMPARISON","TIMING ONLY");
            line(31,"A edges",String(baseline.count));line(51,"B edges",String(second.count));
            int pct=pulseSimilarity(baseline,second);
            line(75,"Timing similarity",pct<0?"Not enough edges":String(pct)+" %",pct<0?AMBER:CYAN);
            tft.setTextColor(DIM,BG);tft.drawString("Not identity or rolling-code match",9,113,1);hold();
        }},
        {"Save captures to SD",[&](){
            if(!baseline.success&&!second.success){info("Take a capture first");return;}
            String out;if(baseline.success)out+=pulseCsv("A",baseline);
            if(second.success)out+=pulseCsv("B",second);
            saveText("/esptrix_v8_keyfob.csv",out);
        }}
    });
}
void v8PulseScope(){
    if(!rfPresent())return;
    float mhz=433.92f;
    PulseCapture current;unsigned zoom=1;
    menu("RF PULSE SCOPE",{
        {"315 MHz - start scope",[&](){mhz=315.00f;capturePulse(current,mhz);showCaptured(current,zoom);}},
        {"433.92 MHz - start scope",[&](){mhz=433.92f;capturePulse(current,mhz);showCaptured(current,zoom);}},
        {"868.35 MHz - start scope",[&](){mhz=868.35f;capturePulse(current,mhz);showCaptured(current,zoom);}},
        {"Capture / refresh",[&](){capturePulse(current,mhz);showCaptured(current,zoom);}},
        {"View paused trace",[&](){if(!current.success){info("Capture first");return;}pulseGraph(current,zoom);hold();}},
        {"Zoom: next level",[&](){zoom=(zoom==1?2:zoom==2?4:1);info("Zoom x"+String(zoom));}},
        {"Save trace to SD",[&](){if(!current.success){info("Capture first");return;}saveText("/esptrix_v8_scope.csv",pulseCsv("scope",current));}}
    });
}

void v8InterferenceFinder(){
    if(!rfPresent())return;
    float mhz=433.92f;
    auto survey=[&](){
        clearKeys();
        screen("RF INTERFERENCE FINDER","RSSI / RX");
        line(27,"Frequency",fmhz(mhz),CYAN);
        tft.setTextColor(DIM,BG);tft.drawString("Walk slowly; BACK stops",9,143,1);
        if(!initRfModule("rx",mhz)){displayError("CC1101 init failed",true);return;}
        int minRssi=0,maxRssi=-120,total=0,n=0,last=-120;
        int x=10;const int top=63,bottom=135;
        tft.drawRect(9,61,tftWidth-18,77,GRID);
        uint32_t start=millis();
        while(millis()-start<30000UL){
            if(check(EscPress))break;
            int r=ELECHOUSE_cc1101.getRssi();
            if(n==0||r<minRssi)minRssi=r;
            if(n==0||r>maxRssi)maxRssi=r;
            total+=r;n++;last=r;
            line(43,"RSSI now / peak",String(r)+" / "+String(maxRssi)+" dBm",CYAN);
            // Relative display. No source identification from RSSI alone.
            int bounded=std::max(-105,std::min(-30,r));
            int h=(bounded+105)*(bottom-top-7)/75;
            tft.drawFastVLine(x,bottom-h,std::max(1,h),CYAN);
            if(++x>=tftWidth-11){x=10;tft.fillRect(10,62,tftWidth-20,75,BG);}
            delay(110);
        }
        deinitRfModule();
        screen("INTERFERENCE RESULTS","RX ONLY");
        line(35,"Frequency",fmhz(mhz));
        line(61,"Average RSSI",n?String(total/n)+" dBm":"--");
        line(87,"Lowest / highest",n?String(minRssi)+" / "+String(maxRssi)+" dBm":"--");
        tft.setTextColor(DIM,BG);tft.drawString("RSSI not source identification",9,122,1);
        hold();
        if(n) menu("RSSI SURVEY REPORT", {
            {"Save results to SD",[=](){
                const String row=String(millis())+","+String(mhz,2)+","+String(n)+","+
                    String(total/n)+","+String(minRssi)+","+String(maxRssi)+"\n";
                saveText("/esptrix_v8_rf_survey.csv",row);
            }}
        });
        (void)last;
    };
    menu("RF INTERFERENCE",{
        {"315 MHz - live survey",[&](){mhz=315.00f;survey();}},
        {"433.92 MHz - live survey",[&](){mhz=433.92f;survey();}},
        {"868.35 MHz - live survey",[&](){mhz=868.35f;survey();}},
        {"Run 30-sec RSSI walk",survey},
        {"About",[](){info("Displays relative RF energy around the tuned CC1101 frequency; not a transmitter locator or decoded signal.");}}
    });
}

void v8TpmsInspector(){
    if(!rfPresent())return;
    menu("TPMS PROTOCOL INSPECTOR",{
        {"315 MHz OOK activity",[](){tpmsAt(315.00f);}},
        {"433.92 MHz OOK activity",[](){tpmsAt(433.92f);}},
        {"Decoder status",[](){info("No verified sensor protocol decoder yet. FSK TPMS and pressure/temperature values are NOT decoded.");}}
    });
}

namespace {
struct TagReading {String uid;uint8_t sak=0;uint8_t atqa0=0,atqa1=0;bool valid=false;};
String tagFamily(uint8_t sak){
    if(sak==0x08)return "Classic 1K (likely)";
    if(sak==0x18)return "Classic 4K (likely)";
    if(sak==0x09)return "Classic Mini (likely)";
    if(sak==0x00)return "Type 2 / Ultralight candidate";
    if((sak&0x20)!=0)return "ISO14443-4 candidate";
    return "Unknown Type A";
}
bool readTag(TagReading &out){
    clearKeys();screen("NFC CARD INSPECTOR","READ ONLY");
    line(39,"Status","Present test tag",CYAN);
    PN532 reader(PN532::I2C);
    bool ready=reader.begin();
    if(!ready){releaseI2CBus();displayError("PN532 not ready",true);return false;}
    // Use driver-provided passive identification; never request protected sectors.
    // Reader returns a Type-A UID with ATQA/SAK metadata.
    bool ok=reader.nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
    if(ok)ok=reader.nfc.readDetectedPassiveTargetID();
    if(ok){
        auto &u=reader.nfc.targetUid;
        out=TagReading{};
        for(unsigned i=0;i<u.size && i<10;i++){
            if(i)out.uid+=" ";
            char b[4];snprintf(b,sizeof(b),"%02X",u.uidByte[i]);out.uid+=b;
        }
        out.sak=u.sak;out.atqa0=u.atqaByte[0];out.atqa1=u.atqaByte[1];out.valid=true;
    }
    releaseI2CBus();
    if(!ok){info("No ISO14443-A tag detected");return false;}
    screen("NFC CARD INSPECTOR","IDENTITY ONLY");
    line(29,"UID",out.uid,CYAN);
    line(52,"SAK",String(out.sak,HEX));
    line(75,"ATQA",String(out.atqa0,HEX)+" "+String(out.atqa1,HEX));
    tft.setTextColor(DIM,BG);tft.drawString(tagFamily(out.sak),9,109,1);
    tft.drawString("Family is inferred, not verified",9,129,1);
    hold();return true;
}
}
void v8NfcInspector(){
    TagReading a,b;
    menu("NFC CARD INSPECTOR",{
        {"Inspect tag A",[&](){readTag(a);}},
        {"Inspect tag B",[&](){readTag(b);}},
        {"Compare identifiers",[&](){
            if(!a.valid||!b.valid){info("Read A and B first");return;}
            screen("TAG COMPARISON","UID ONLY");
            line(35,"A UID",a.uid);
            line(60,"B UID",b.uid);
            line(91,"Same UID",a.uid==b.uid?"YES":"NO",a.uid==b.uid?CYAN:AMBER);
            tft.setTextColor(DIM,BG);tft.drawString("Matching UID is not authentication",9,126,1);hold();
        }},
        {"Save tag report",[&](){
            if(!a.valid&&!b.valid){info("Inspect tag first");return;}
            String out="label,uid,sak_hex,atqa_hex,likely_type\n";
            auto add=[&](const char *label,const TagReading &r){if(!r.valid)return;
                out+=String(label)+","+csv(r.uid)+","+String(r.sak,HEX)+","+String(r.atqa0,HEX)+String(r.atqa1,HEX)+","+csv(tagFamily(r.sak))+"\n";};
            add("A",a);add("B",b);saveText("/esptrix_v8_tags.csv",out);
        }}
    });
}

namespace {
struct BleNode {String addr,name;int rssi=0;};
void watchBle(const BleNode &target){
    NimBLEScan *scan=NimBLEDevice::getScan();
    if(!scan){displayError("BLE scanner unavailable",true);return;}
    screen("BLE DEVICE LOCATOR","RSSI ONLY");
    tft.setTextColor(ESPTV8_WHITE,BG);tft.drawString(target.name.substring(0,24),9,31,1);
    tft.drawString(target.addr,9,47,1);
    tft.drawRect(9,76,tftWidth-18,62,GRID);
    unsigned seen=0,missed=0; int best=-127,x=10;
    clearKeys();
    while(!check(EscPress)) {
        // NimBLE returns advertisement results, not distance or physical location.
        NimBLEScanResults found=scan->getResults(1700,false);
        bool seenNow=false;int rssi=-127;
        for(int i=0;i<found.getCount();i++){
            const NimBLEAdvertisedDevice *d=found.getDevice(i);
            if(!d)continue;
            if(String(d->getAddress().toString().c_str())==target.addr) {seenNow=true;rssi=d->getRSSI();break;}
        }
        scan->clearResults();
        if(seenNow){seen++;best=std::max(best,rssi);}else missed++;
        line(61,"RSSI / best",seenNow?String(rssi)+" / "+String(best)+" dBm":"Not seen",seenNow?CYAN:AMBER);
        const int h=seenNow?std::max(1,(std::max(-100,std::min(-30,rssi))+100)*52/70):1;
        tft.drawFastVLine(x,135-h,h,seenNow?CYAN:DIM);
        if(++x>=tftWidth-11){x=10;tft.fillRect(10,77,tftWidth-20,60,BG);}
        if(check(SelPress)) break;
    }
    scan->stop();scan->clearResults();clearKeys();
    screen("BLE LOCATOR RESULTS","RX ONLY");
    line(35,"Seen scans",String(seen));
    line(59,"Missed scans",String(missed));
    line(83,"Best signal",seen?String(best)+" dBm":"--");
    tft.setTextColor(DIM,BG);tft.drawString("RSSI is NOT an accurate distance",9,119,1);
    hold();
}
}
void v8BleLocator(){
    // Leave the BLE stack running after scanning. It may be in use by Chameleon.
    NimBLEDevice::init("ESPTRIX-Locator");
    NimBLEScan *scan=NimBLEDevice::getScan();
    if(!scan){displayError("BLE scanner unavailable",true);return;}
    scan->setActiveScan(false);
    screen("BLE DEVICE LOCATOR","DISCOVERY");
    line(50,"Status","Scanning nearby tags",CYAN);
    auto results=scan->getResults(3500,false);
    std::vector<BleNode> devices;
    for(int i=0;i<results.getCount() && devices.size()<25;i++){
        const NimBLEAdvertisedDevice *dev=results.getDevice(i);
        if(!dev)continue;
        BleNode node;
        node.addr=String(dev->getAddress().toString().c_str());
        node.name=String(dev->getName().c_str());
        if(node.name.length()==0)node.name="(Unnamed BLE)";
        node.rssi=dev->getRSSI();devices.push_back(node);
    }
    scan->clearResults();
    if(devices.empty()){info("No BLE advertisements discovered");return;}
    std::vector<Option> choices;
    for(const auto &device:devices){
        const String label=device.name.substring(0,17)+" "+String(device.rssi)+"dBm";
        choices.push_back({label,[device](){watchBle(device);}});
    }
    menu("SELECT BLE DEVICE",choices);
}

namespace {
struct WifiTarget{String ssid,bssid;int channel=0;};
struct SurveyPoint{String label;int rssi=-127;uint32_t seconds=0;};
struct WifiRadioGuard {
    bool wasOff;
    WifiRadioGuard():wasOff(WiFi.getMode()==WIFI_OFF){if(wasOff)WiFi.mode(WIFI_STA);}
    ~WifiRadioGuard(){WiFi.scanDelete();if(wasOff)WiFi.mode(WIFI_OFF);}
};
int wifiTargetRssi(const WifiTarget &target) {
    // Fresh snapshot. BSSID distinguishes an AP from others on the same SSID.
    const int count=WiFi.scanNetworks(false,true);
    int result=-127;
    if(count>=0)for(int i=0;i<count;i++){
        if(WiFi.BSSIDstr(i)==target.bssid){result=WiFi.RSSI(i);break;}
    }
    WiFi.scanDelete();return result;
}
// A stable visual signal meter; RSSI is only a rough guide to coverage.
void walkPointDisplay(const WifiTarget &target,const SurveyPoint &p,size_t count){
    screen("WI-FI WALK TEST","2.4 GHz");
    const String name=p.label.length()>24?p.label.substring(0,24):p.label;
    line(29,"Location",name,CYAN);
    const String ssid=target.ssid.length()>20?target.ssid.substring(0,20):target.ssid;
    line(48,"Access point",ssid);
    line(67,"Channel",String(target.channel));
    const int r=p.rssi;
    const bool detected=r!=-127;
    line(86,"Signal",detected?String(r)+" dBm":"NOT DETECTED",detected?CYAN:AMBER);
    const char *quality = !detected?"NOT SEEN":r>=-55?"STRONG":r>=-67?"GOOD":r>=-78?"FAIR":"WEAK";
    tft.fillRect(10,109,tftWidth-20,14,BG);
    tft.setTextColor(DIM,BG);tft.drawString(quality,10,110,1);
    const int bx=94,by=111,bw=tftWidth-108;
    tft.drawRect(bx,by,bw,10,GRID);
    const int amount=detected?std::max(0,std::min(100,(r+100)*100/65)):0;
    const uint16_t colour=!detected||r< -78?AMBER:CYAN;
    if(amount>0)tft.fillRect(bx+1,by+1,(bw-2)*amount/100,8,colour);
    line(130,"Saved points",String(count));
}
void walkWifi(const WifiTarget &target){
    std::vector<SurveyPoint> spots;
    auto addPoint=[&](){
        if(spots.size()>=12){info("Limit: 12 measurement points");return;}
        String suggested="Point "+String(spots.size()+1);
        String name=keyboard(suggested,16,"Name this location:");
        if(!name.length())name=suggested;
        screen("WI-FI WALK TEST","2.4 GHz");
        line(40,"Location",name);
        line(67,"Status","Fresh Wi-Fi scan",CYAN);
        int r=wifiTargetRssi(target);
        spots.push_back({name,r,millis()/1000});
        walkPointDisplay(target,spots.back(),spots.size());
        hold();
    };
    auto results=[&](){
        if(spots.empty()){info("Take a measurement first");return;}
        std::vector<Option> rows;
        for(const auto &p:spots){
            String label=p.label.substring(0,18)+" "+(p.rssi==-127?"N/A":String(p.rssi)+" dBm");
            rows.push_back({label,[&,p](){walkPointDisplay(target,p,spots.size());hold();}});
        }
        menu("WALK TEST RESULTS",rows);
    };
    auto exportCsv=[&](){
        if(spots.empty()){info("No measurements yet");return;}
        String out="ssid,bssid,location,rssi_dbm,uptime_s\n";
        for(const auto &p:spots){
            out+=csv(target.ssid)+","+csv(target.bssid)+","+csv(p.label)+",";
            if(p.rssi!=-127)out+=String(p.rssi);
            out+=","+String(p.seconds)+"\n";
        }
        saveText("/esptrix_v8_wifi_walk.csv",out);
    };
    menu("WI-FI WALK TEST",{
        {"Measure next location",addPoint},
        {"Review saved points",results},
        {"Export walk test to SD",exportCsv},
        {"About",[](){info("2.4 GHz detected access-point RSSI only. Walk to each room and record a fresh scan.");}}
    });
}
}
void v8WifiWalkSurvey(){
    WifiRadioGuard wifi;
    screen("WI-FI WALK TEST","SELECT AP");
    line(48,"Status","Scanning nearby APs",CYAN);
    const int found=WiFi.scanNetworks(false,true);
    std::vector<WifiTarget> targets;
    if(found>=0)for(int i=0;i<found&&targets.size()<25;i++){
        int channel=WiFi.channel(i);
        // ESP32-S3 2.4GHz Wi-Fi only; do not misrepresent a 5GHz survey.
        if(channel<1||channel>14)continue;
        targets.push_back({WiFi.SSID(i),WiFi.BSSIDstr(i),channel});
    }
    WiFi.scanDelete();
    if(targets.empty()){info("No 2.4 GHz access points found");return;}
    std::vector<Option> rows;
    for(const auto &t:targets){
        String label=(t.ssid.length()?t.ssid:"(hidden)").substring(0,20)+" CH"+String(t.channel);
        rows.push_back({label,[t](){walkWifi(t);}});
    }
    menu("SELECT ACCESS POINT",rows);
}

void v8AcousticSpectrum(){
#if defined(MIC_SPM1423) || defined(MIC_INMP441)
    // Coarse frequency-band analysis, not a calibrated SPL meter.
    const float bands[]={125,250,500,1000,2000,3000,4000,6000};
    screen("ACOUSTIC ANALYSER","MIC / RX");
    clearKeys();
    while(!check(EscPress)){
        int16_t *pcm=nullptr;uint32_t sampleRate=0;
        if(!mic_capture_samples(1024,16000,1.0f,&pcm,&sampleRate)||!pcm||sampleRate==0){
            if(pcm)free(pcm);displayError("Microphone capture failed",true);return;
        }
        // Remove DC and apply a Hann window before Goertzel bins.
        double mean=0;
        for(int i=0;i<1024;i++)mean+=pcm[i];
        mean/=1024.0;
        double powers[8]={};double best=0;int bestIndex=-1;
        for(int b=0;b<8;b++){
            const double omega=2.0*3.141592653589793*bands[b]/sampleRate;
            const double coeff=2.0*cos(omega);
            double s0=0,s1=0,s2=0;
            for(int j=0;j<1024;j++){
                const double window=0.5-0.5*cos(2.0*3.141592653589793*j/1023.0);
                s0=(static_cast<double>(pcm[j])-mean)*window+coeff*s1-s2;
                s2=s1;s1=s0;
            }
            double p=s1*s1+s2*s2-coeff*s1*s2;
            powers[b]=std::max(0.0,p);
            if(powers[b]>best){best=powers[b];bestIndex=b;}
        }
        free(pcm);
        // No full-screen redrawing while listening.
        tft.fillRect(8,24,tftWidth-16,116,BG);
        tft.setTextColor(ESPTV8_WHITE,BG);
        tft.drawString("Strongest measured band: "+(bestIndex>=0?String(static_cast<int>(bands[bestIndex]))+" Hz":"--"),10,30,1);
        tft.setTextColor(DIM,BG);tft.drawString("Relative level (not dB SPL)",10,45,1);
        const int base=122,width=24,space=13;
        for(int i=0;i<8;i++){
            double proportion=(best>0)?sqrt(powers[i]/best):0;
            int height=std::max(1,static_cast<int>(proportion*51));
            int x=10+i*(width+space);
            tft.fillRect(x,base-height,width,height,(i==bestIndex)?CYAN:BLUE);
            tft.setTextColor(DIM,BG);
            tft.drawString(String(static_cast<int>(bands[i])),x,128,1);
        }
        if(check(SelPress))break;
    }
    clearKeys();
#else
    displayError("Microphone not compiled for this board",true);
#endif
}
} // namespace esptrix
#else
namespace esptrix {
void v8KeyFobAnalyzer(){} void v8PulseScope(){} void v8InterferenceFinder(){}
void v8TpmsInspector(){} void v8NfcInspector(){} void v8BleLocator(){}
void v8WifiWalkSurvey(){} void v8AcousticSpectrum(){}
}
#endif
