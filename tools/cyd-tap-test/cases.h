// ชุดทดสอบการแตะของ Host บอร์ด CYD (ใช้ร่วมกันทั้งแนวนอนและแนวตั้ง) ถูก #include ต่อท้ายตัวขับเรนเดอร์
static int bad=0;
#define CK(c,m) do{ if(!(c)){printf("ผิด: %s\n",m);bad++;} else printf("ผ่าน: %s\n",m);}while(0)
static TouchZone mid(const TouchZone&z){ return TouchZone{z.x+z.w/2,z.y+z.h/2,0,0}; }
int main(){
  g_ops=fopen("/dev/null","w"); setup();
  activeStationCount=5; stations[0].active=true;
  // หน้า BED SET: แตะ - + ของทั้ง 4 ช่อง และส่วนค่า
  uiSelectedBed=0; uiScreen=SCR_BED_SET;
  for(int f=0;f<4;f++){
    int cx=SET_ROW_X[f], cy=SET_ROW_Y[f];
    editTargetRate=100; editPlanVolume=1000; editDropFactor=20; editNearPct=80;
    float before[4]={editTargetRate,editPlanVolume,(float)editDropFactor,(float)editNearPct};
    handleBedSetTap(cx+SET_P_X+SET_P_W/2, cy+SET_ROW_H/2);
    float after[4]={editTargetRate,editPlanVolume,(float)editDropFactor,(float)editNearPct};
    char m[80]; snprintf(m,80,"BED SET ช่อง %d: แตะ + เพิ่มค่าเฉพาะช่องนั้น",f);
    bool ok=true; for(int k=0;k<4;k++) if((k==f)!=(after[k]!=before[k])) ok=false;
    CK(ok,m);
    uiScreen=SCR_BED_SET;
    handleBedSetTap(cx+SET_M_X+SET_M_W/2, cy+SET_ROW_H/2);
    snprintf(m,80,"BED SET ช่อง %d: แตะ - ลดกลับ",f);
    float back[4]={editTargetRate,editPlanVolume,(float)editDropFactor,(float)editNearPct};
    ok=true; for(int k=0;k<4;k++) if(back[k]!=before[k]) ok=false; CK(ok,m);
    uiScreen=SCR_BED_SET;
    handleBedSetTap(cx+10, cy+SET_ROW_H/2);
    snprintf(m,80,"BED SET ช่อง %d: แตะค่า = เปิดแป้นตัวเลข",f);
    CK(uiScreen==SCR_NUMPAD && uiEditField==f,m);
    uiScreen=SCR_BED_SET;
  }
  { TouchZone z=mid(ZONE_SET_SAVE); uiScreen=SCR_BED_SET; handleBedSetTap(z.x,z.y); CK(uiScreen==SCR_BED,"SAVE กลับหน้า BED"); }
  { TouchZone z=mid(ZONE_SET_CANCEL); uiScreen=SCR_BED_SET; handleBedSetTap(z.x,z.y); CK(uiScreen==SCR_BED,"CANCEL กลับหน้า BED"); }
  // หน้า BED
  { TouchZone z=mid(ZONE_BED_SET); uiScreen=SCR_BED; handleBedTap(z.x,z.y); CK(uiScreen==SCR_BED_SET,"ปุ่ม SET ในหน้า BED"); }
  { TouchZone z=mid(ZONE_BED_ACK); uiScreen=SCR_BED; stations[0].nearEndAck=false; handleBedTap(z.x,z.y); CK(stations[0].nearEndAck,"ปุ่ม ACK"); }
  { TouchZone z=mid(ZONE_BED_NEWBAG); uiScreen=SCR_BED; uint8_t r=stations[0].cfg.resetSeq; handleBedTap(z.x,z.y); CK(stations[0].cfg.resetSeq!=r,"ปุ่ม NEW BAG"); }
  // แป้นตัวเลข: พิมพ์ 1 2 5 แล้ว OK
  { uiEditField=0; openNumpad(0); editTargetRate=0;
    int keys[4]={0,1,4,11};
    for(int k:keys){ TouchZone z=mid(numpadKeyZone(k)); handleNumpadTap(z.x,z.y); }
    CK(editTargetRate==125 && uiScreen==SCR_BED_SET,"แป้นตัวเลข: พิมพ์ 125 + OK"); }
  { TouchZone z=mid(numpadKeyZone(9)); openNumpad(0); handleNumpadTap(mid(numpadKeyZone(0)).x,mid(numpadKeyZone(0)).y); handleNumpadTap(z.x,z.y); CK(editNumBuf[0]=='\0',"แป้นตัวเลข: ปุ่ม C ล้างค่า"); }
  // หน้า SYSTEM
  { uiScreen=SCR_SYS; activeStationCount=5; handleSysTap(SYS_CELL_X[0]+SET_P_X+5, SYS_CELL_Y[0]+SET_ROW_H/2); CK(activeStationCount==6,"SYSTEM: จำนวนเตียง +"); }
  { uiScreen=SCR_SYS; handleSysTap(SYS_CELL_X[0]+SET_M_X+5, SYS_CELL_Y[0]+SET_ROW_H/2); CK(activeStationCount==5,"SYSTEM: จำนวนเตียง -"); }
  { uiScreen=SCR_SYS; screenBrightness=50; handleSysTap(SYS_CELL_X[1]+SET_P_X+5, SYS_CELL_Y[1]+SET_ROW_H/2); CK(screenBrightness==75,"SYSTEM: ความสว่าง +"); }
  { uiScreen=SCR_SYS; handleSysTap(SYS_CELL_X[1]+SET_M_X+5, SYS_CELL_Y[1]+SET_ROW_H/2); CK(screenBrightness==50,"SYSTEM: ความสว่าง -"); }
  { TouchZone z=mid(ZONE_SYS_CALIB); uiScreen=SCR_SYS; handleSysTap(z.x,z.y); CK(uiScreen==SCR_CALIB,"SYSTEM: ปุ่ม CALIBRATION"); }
  // หน้า HOME
  { TouchZone z=mid(ZONE_GEAR); uiScreen=SCR_HOME; handleHomeTap(z.x,z.y); CK(uiScreen==SCR_SYS,"HOME: ปุ่ม SET"); }
  { TouchZone z=mid(ZONE_FOCUS); uiScreen=SCR_HOME; handleHomeTap(z.x,z.y); CK(uiScreen==SCR_BED,"HOME: แตะการ์ดโฟกัส"); }
  { activeStationCount=5; for(int i=0;i<5;i++){stations[i].active=true;} uiScreen=SCR_HOME; uiNeedFramework=true; updateHostDisplay(); updateHostDisplay();
    CK(homeRowCount==4,"HOME: มีแถวรายการ 4 แถว");
    for(int r=0;r<homeRowCount;r++){
      int y=LIST_Y+r*(homeRowH+4)+homeRowH/2; uiScreen=SCR_HOME; handleHomeTap(LIST_X+LIST_W/2,y);
      char m[60]; snprintf(m,60,"HOME: แตะแถวที่ %d เปิดเตียง %d",r,homeRowBed[r]+1); CK(uiScreen==SCR_BED && uiSelectedBed==homeRowBed[r],m);
    }
#if UI_LANDSCAPE
    uiScreen=SCR_HOME; handleHomeTap(20,LIST_Y+5); /* ซ้ายของรายการ = โซนโฟกัส ไม่ใช่แถว */
    CK(uiSelectedBed==currentFocusBed(),"HOME: แตะซีกซ้ายไม่ไปเปิดแถวรายการ");
#endif
  }
  printf(bad?"\nไม่ผ่าน %d ข้อ\n":"\nผ่านทุกข้อ\n",bad); return bad?1:0;
}
