#include <Fuzzy.h>
#include <OneWire.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <EEPROM.h>
#include "DFRobot_ESP_EC.h"
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 13 // this is the gpio pin 13 on esp32.
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define Tds_Pin A0
DFRobot_ESP_EC ec;

#define ph_Pin 39
float PH_step, Po = 0;
float voltage, temperature = 25, ecValue;
int nilai_analog_PH, nilai_analog_EC;
double TeganganPh;

//pin pompa
#define pompainon 2
#define pompaouton 15

//untuk kalibrasi
float PH4 = 3.30;
float PH7 = 2.54;

const char *ssid = "V2038";
const char *password = "0tujuhkali";
const char *host = "192.168.126.237";
unsigned long previousMillis = 0;
const long interval = 1000;

float readTemperature()
{
  //add your code here to get the temperature from your temperature sensor
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

Fuzzy *fuzzy = new Fuzzy();

// fuzzy input
FuzzyInput *ph = new FuzzyInput(1);
FuzzySet *asam = new FuzzySet(0, 0, 4, 6);
FuzzySet *netral = new FuzzySet(4, 7, 7, 9);
FuzzySet *basa = new FuzzySet(8, 9, 15, 15);

FuzzyInput *temp = new FuzzyInput(2);
FuzzySet *rendah = new FuzzySet(0, 0, 11, 22);
FuzzySet *sedang = new FuzzySet(11, 22, 27, 33);
FuzzySet *tinggi = new FuzzySet(27, 33, 50, 50);

FuzzyInput *sal = new FuzzyInput(3);
FuzzySet *tawar = new FuzzySet(0, 0, 0.25, 0.5);
FuzzySet *payau = new FuzzySet(0.25, 0.5, 14, 30);
FuzzySet *asn = new FuzzySet(14, 30, 50, 50);

//fuzzy out
FuzzyOutput *pumpin = new FuzzyOutput(1);
FuzzySet *in_on = new FuzzySet(0, 0, 50, 100);
FuzzySet *in_off = new FuzzySet(50, 100, 150, 150);

FuzzyOutput *pumpout = new FuzzyOutput(2);
FuzzySet *out_on = new FuzzySet(0, 0, 50, 100);
FuzzySet *out_off = new FuzzySet(50, 100, 150, 150);

void rule()
{
  ph->addFuzzySet(asam);
  ph->addFuzzySet(netral);
  ph->addFuzzySet(basa);
  fuzzy->addFuzzyInput(ph);

  temp->addFuzzySet(rendah);
  temp->addFuzzySet(sedang);
  temp->addFuzzySet(tinggi);
  fuzzy->addFuzzyInput(temp);

  sal->addFuzzySet(tawar);
  sal->addFuzzySet(payau);
  sal->addFuzzySet(asn);
  fuzzy->addFuzzyInput(sal);

  pumpin->addFuzzySet(in_on);
  pumpin->addFuzzySet(in_off);
  fuzzy->addFuzzyOutput(pumpin);

  pumpout->addFuzzySet(out_on);
  pumpout->addFuzzySet(out_off);
  fuzzy->addFuzzyOutput(pumpout);

  //tawar
  FuzzyRuleAntecedent *tawarANDrendah1 = new FuzzyRuleAntecedent();
  tawarANDrendah1->joinWithAND(tawar, rendah);
  FuzzyRuleAntecedent *asam_1 = new FuzzyRuleAntecedent();
  asam_1->joinSingle(asam);
  FuzzyRuleAntecedent *tawar_rendahANDasam = new FuzzyRuleAntecedent();
  tawar_rendahANDasam->joinWithAND(tawarANDrendah1, asam_1);
  FuzzyRuleConsequent *then_inoff_AND_outoff1 = new FuzzyRuleConsequent();
  then_inoff_AND_outoff1->addOutput(in_off);
  then_inoff_AND_outoff1->addOutput(out_off);
  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, tawar_rendahANDasam, then_inoff_AND_outoff1);
  fuzzy->addFuzzyRule(fuzzyRule1);

  FuzzyRuleAntecedent *tawarANDsedang1 = new FuzzyRuleAntecedent();
  tawarANDsedang1->joinWithAND(tawar, sedang);
  FuzzyRuleAntecedent *asam_2 = new FuzzyRuleAntecedent();
  asam_2->joinSingle(asam);
  FuzzyRuleAntecedent *tawar_sedangANDasam = new FuzzyRuleAntecedent();
  tawar_sedangANDasam->joinWithAND(tawarANDsedang1, asam_2);
  FuzzyRuleConsequent *then_inoff_AND_outoff2 = new FuzzyRuleConsequent();
  then_inoff_AND_outoff2->addOutput(in_off);
  then_inoff_AND_outoff2->addOutput(out_off);
  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, tawar_sedangANDasam, then_inoff_AND_outoff2);
  fuzzy->addFuzzyRule(fuzzyRule2);

  FuzzyRuleAntecedent *tawarANDtinggi1 = new FuzzyRuleAntecedent();
  tawarANDtinggi1->joinWithAND(tawar, tinggi);
  FuzzyRuleAntecedent *asam_3 = new FuzzyRuleAntecedent();
  asam_3->joinSingle(asam);
  FuzzyRuleAntecedent *tawar_tinggiANDasam = new FuzzyRuleAntecedent();
  tawar_tinggiANDasam->joinWithAND(tawarANDtinggi1, asam_3);
  FuzzyRuleConsequent *then_inoff_AND_outoff3 = new FuzzyRuleConsequent();
  then_inoff_AND_outoff3->addOutput(in_off);
  then_inoff_AND_outoff3->addOutput(out_off);
  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, tawar_tinggiANDasam, then_inoff_AND_outoff3);
  fuzzy->addFuzzyRule(fuzzyRule3);

  FuzzyRuleAntecedent *tawarANDrendah2 = new FuzzyRuleAntecedent();
  tawarANDrendah2->joinWithAND(tawar, rendah);
  FuzzyRuleAntecedent *netral_1 = new FuzzyRuleAntecedent();
  netral_1->joinSingle(netral);
  FuzzyRuleAntecedent *tawar_rendahANDnetral = new FuzzyRuleAntecedent();
  tawar_rendahANDnetral->joinWithAND(tawarANDrendah2, netral_1);
  FuzzyRuleConsequent *then_inoff_AND_outoff4 = new FuzzyRuleConsequent();
  then_inoff_AND_outoff4->addOutput(in_off);
  then_inoff_AND_outoff4->addOutput(out_off);
  FuzzyRule *fuzzyRule4 = new FuzzyRule(4, tawar_rendahANDnetral, then_inoff_AND_outoff4);
  fuzzy->addFuzzyRule(fuzzyRule4);

  FuzzyRuleAntecedent *tawarANDsedang2 = new FuzzyRuleAntecedent();
  tawarANDsedang2->joinWithAND(tawar, sedang);
  FuzzyRuleAntecedent *netral_2 = new FuzzyRuleAntecedent();
  netral_2->joinSingle(netral);
  FuzzyRuleAntecedent *tawar_sedangANDnetral = new FuzzyRuleAntecedent();
  tawar_sedangANDnetral->joinWithAND(tawarANDsedang2, netral_2);
  FuzzyRuleConsequent *then_inoff_AND_outoff5 = new FuzzyRuleConsequent();
  then_inoff_AND_outoff5->addOutput(in_off);
  then_inoff_AND_outoff5->addOutput(out_off);
  FuzzyRule *fuzzyRule5 = new FuzzyRule(5, tawar_sedangANDnetral, then_inoff_AND_outoff5);
  fuzzy->addFuzzyRule(fuzzyRule5);

  FuzzyRuleAntecedent *tawarANDtinggi2 = new FuzzyRuleAntecedent();
  tawarANDtinggi2->joinWithAND(tawar, tinggi);
  FuzzyRuleAntecedent *netral_3 = new FuzzyRuleAntecedent();
  netral_3->joinSingle(netral);
  FuzzyRuleAntecedent *tawar_tinggiANDnetral = new FuzzyRuleAntecedent();
  tawar_tinggiANDnetral->joinWithAND(tawarANDtinggi2, netral_3);
  FuzzyRuleConsequent *then_inoff_AND_outoff6 = new FuzzyRuleConsequent();
  then_inoff_AND_outoff6->addOutput(in_off);
  then_inoff_AND_outoff6->addOutput(out_off);
  FuzzyRule *fuzzyRule6 = new FuzzyRule(6, tawar_tinggiANDnetral, then_inoff_AND_outoff6);
  fuzzy->addFuzzyRule(fuzzyRule6);

  FuzzyRuleAntecedent *tawarANDrendah3 = new FuzzyRuleAntecedent();
  tawarANDrendah3->joinWithAND(tawar, rendah);
  FuzzyRuleAntecedent *basa_1 = new FuzzyRuleAntecedent();
  basa_1->joinSingle(basa);
  FuzzyRuleAntecedent *tawar_rendahANDbasa = new FuzzyRuleAntecedent();
  tawar_rendahANDbasa->joinWithAND(tawarANDrendah3, basa_1);
  FuzzyRuleConsequent *then_inoff_AND_outoff7 = new FuzzyRuleConsequent();
  then_inoff_AND_outoff7->addOutput(in_off);
  then_inoff_AND_outoff7->addOutput(out_off);
  FuzzyRule *fuzzyRule7 = new FuzzyRule(7, tawar_rendahANDbasa, then_inoff_AND_outoff7);
  fuzzy->addFuzzyRule(fuzzyRule7);

  FuzzyRuleAntecedent *tawarANDsedang3 = new FuzzyRuleAntecedent();
  tawarANDsedang3->joinWithAND(tawar, sedang);
  FuzzyRuleAntecedent *basa_2 = new FuzzyRuleAntecedent();
  basa_2->joinSingle(basa);
  FuzzyRuleAntecedent *tawar_sedangANDbasa = new FuzzyRuleAntecedent();
  tawar_sedangANDbasa->joinWithAND(tawarANDsedang3, basa_2);
  FuzzyRuleConsequent *then_inoff_AND_outoff8 = new FuzzyRuleConsequent();
  then_inoff_AND_outoff8->addOutput(in_off);
  then_inoff_AND_outoff8->addOutput(out_off);
  FuzzyRule *fuzzyRule8 = new FuzzyRule(8, tawar_sedangANDbasa, then_inoff_AND_outoff8);
  fuzzy->addFuzzyRule(fuzzyRule8);

  FuzzyRuleAntecedent *tawarANDtinggi3 = new FuzzyRuleAntecedent();
  tawarANDtinggi3->joinWithAND(tawar, tinggi);
  FuzzyRuleAntecedent *basa_3 = new FuzzyRuleAntecedent();
  basa_3->joinSingle(basa);
  FuzzyRuleAntecedent *tawar_tinggiANDbasa = new FuzzyRuleAntecedent();
  tawar_tinggiANDbasa->joinWithAND(tawarANDtinggi3, basa_3);
  FuzzyRuleConsequent *then_inoff_AND_outoff9 = new FuzzyRuleConsequent();
  then_inoff_AND_outoff9->addOutput(in_off);
  then_inoff_AND_outoff9->addOutput(out_off);
  FuzzyRule *fuzzyRule9 = new FuzzyRule(9, tawar_tinggiANDbasa, then_inoff_AND_outoff9);
  fuzzy->addFuzzyRule(fuzzyRule9);

  //payau

  FuzzyRuleAntecedent *payauANDrendah1 = new FuzzyRuleAntecedent();
  payauANDrendah1->joinWithAND(payau, rendah);
  FuzzyRuleAntecedent *asam_4 = new FuzzyRuleAntecedent();
  asam_4->joinSingle(asam);
  FuzzyRuleAntecedent *payau_rendahANDasam = new FuzzyRuleAntecedent();
  payau_rendahANDasam->joinWithAND(payauANDrendah1, asam_4);
  FuzzyRuleConsequent *then_inoff_AND_outon1 = new FuzzyRuleConsequent();
  then_inoff_AND_outon1->addOutput(in_off);
  then_inoff_AND_outon1->addOutput(out_on);
  FuzzyRule *fuzzyRule10 = new FuzzyRule(10, payau_rendahANDasam, then_inoff_AND_outon1);
  fuzzy->addFuzzyRule(fuzzyRule10);

  FuzzyRuleAntecedent *payauANDsedang1 = new FuzzyRuleAntecedent();
  payauANDsedang1->joinWithAND(payau, sedang);
  FuzzyRuleAntecedent *asam_5 = new FuzzyRuleAntecedent();
  asam_5->joinSingle(asam);
  FuzzyRuleAntecedent *payau_sedangANDasam = new FuzzyRuleAntecedent();
  payau_sedangANDasam->joinWithAND(payauANDsedang1, asam_5);
  FuzzyRuleConsequent *then_inoff_AND_outon2 = new FuzzyRuleConsequent();
  then_inoff_AND_outon2->addOutput(in_off);
  then_inoff_AND_outon2->addOutput(out_on);
  FuzzyRule *fuzzyRule11 = new FuzzyRule(11, payau_sedangANDasam, then_inoff_AND_outon2);
  fuzzy->addFuzzyRule(fuzzyRule11);

  FuzzyRuleAntecedent *payauANDtinggi1 = new FuzzyRuleAntecedent();
  payauANDtinggi1->joinWithAND(payau, tinggi);
  FuzzyRuleAntecedent *asam_6 = new FuzzyRuleAntecedent();
  asam_6->joinSingle(asam);
  FuzzyRuleAntecedent *payau_tinggiANDasam = new FuzzyRuleAntecedent();
  payau_tinggiANDasam->joinWithAND(payauANDtinggi1, asam_6);
  FuzzyRuleConsequent *then_inoff_AND_outon3 = new FuzzyRuleConsequent();
  then_inoff_AND_outon3->addOutput(in_off);
  then_inoff_AND_outon3->addOutput(out_on);
  FuzzyRule *fuzzyRule12 = new FuzzyRule(12, payau_tinggiANDasam, then_inoff_AND_outon3);
  fuzzy->addFuzzyRule(fuzzyRule12);

  FuzzyRuleAntecedent *payauANDrendah2 = new FuzzyRuleAntecedent();
  payauANDrendah2->joinWithAND(payau, rendah);
  FuzzyRuleAntecedent *netral_4 = new FuzzyRuleAntecedent();
  netral_4->joinSingle(netral);
  FuzzyRuleAntecedent *payau_rendahANDnetral = new FuzzyRuleAntecedent();
  payau_rendahANDnetral->joinWithAND(payauANDrendah2, netral_4);
  FuzzyRuleConsequent *then_inoff_AND_outon4 = new FuzzyRuleConsequent();
  then_inoff_AND_outon4->addOutput(in_off);
  then_inoff_AND_outon4->addOutput(out_on);
  FuzzyRule *fuzzyRule13 = new FuzzyRule(13, payau_rendahANDnetral, then_inoff_AND_outon4);
  fuzzy->addFuzzyRule(fuzzyRule13);

  FuzzyRuleAntecedent *payauANDsedang2 = new FuzzyRuleAntecedent();
  payauANDsedang2->joinWithAND(payau, sedang);
  FuzzyRuleAntecedent *netral_5 = new FuzzyRuleAntecedent();
  netral_5->joinSingle(netral);
  FuzzyRuleAntecedent *payau_sedangANDnetral = new FuzzyRuleAntecedent();
  payau_sedangANDnetral->joinWithAND(payauANDsedang2, netral_5);
  FuzzyRuleConsequent *then_inoff_AND_outon5 = new FuzzyRuleConsequent();
  then_inoff_AND_outon5->addOutput(in_off);
  then_inoff_AND_outon5->addOutput(out_on);
  FuzzyRule *fuzzyRule14 = new FuzzyRule(14, payau_sedangANDnetral, then_inoff_AND_outon5);
  fuzzy->addFuzzyRule(fuzzyRule14);

  FuzzyRuleAntecedent *payauANDtinggi2 = new FuzzyRuleAntecedent();
  payauANDtinggi2->joinWithAND(payau, tinggi);
  FuzzyRuleAntecedent *netral_6 = new FuzzyRuleAntecedent();
  netral_6->joinSingle(netral);
  FuzzyRuleAntecedent *payau_tinggiANDnetral = new FuzzyRuleAntecedent();
  payau_tinggiANDnetral->joinWithAND(payauANDtinggi2, netral_6);
  FuzzyRuleConsequent *then_inoff_AND_outon6 = new FuzzyRuleConsequent();
  then_inoff_AND_outon6->addOutput(in_off);
  then_inoff_AND_outon6->addOutput(out_on);
  FuzzyRule *fuzzyRule15 = new FuzzyRule(15, payau_tinggiANDnetral, then_inoff_AND_outon6);
  fuzzy->addFuzzyRule(fuzzyRule15);

  FuzzyRuleAntecedent *payauANDrendah3 = new FuzzyRuleAntecedent();
  payauANDrendah3->joinWithAND(payau, rendah);
  FuzzyRuleAntecedent *basa_4 = new FuzzyRuleAntecedent();
  basa_4->joinSingle(basa);
  FuzzyRuleAntecedent *payau_rendahANDbasa = new FuzzyRuleAntecedent();
  payau_rendahANDbasa->joinWithAND(payauANDrendah3, basa_4);
  FuzzyRuleConsequent *then_inoff_AND_outon7 = new FuzzyRuleConsequent();
  then_inoff_AND_outon7->addOutput(in_off);
  then_inoff_AND_outon7->addOutput(out_on);
  FuzzyRule *fuzzyRule16 = new FuzzyRule(16, payau_rendahANDbasa, then_inoff_AND_outon7);
  fuzzy->addFuzzyRule(fuzzyRule16);

  FuzzyRuleAntecedent *payauANDsedang3 = new FuzzyRuleAntecedent();
  payauANDsedang3->joinWithAND(payau, sedang);
  FuzzyRuleAntecedent *basa_5 = new FuzzyRuleAntecedent();
  basa_5->joinSingle(basa);
  FuzzyRuleAntecedent *payau_sedangANDbasa = new FuzzyRuleAntecedent();
  payau_sedangANDbasa->joinWithAND(payauANDsedang3, basa_5);
  FuzzyRuleConsequent *then_inoff_AND_outon8 = new FuzzyRuleConsequent();
  then_inoff_AND_outon8->addOutput(in_off);
  then_inoff_AND_outon8->addOutput(out_on);
  FuzzyRule *fuzzyRule17 = new FuzzyRule(17, payau_sedangANDbasa, then_inoff_AND_outon8);
  fuzzy->addFuzzyRule(fuzzyRule17);

  FuzzyRuleAntecedent *payauANDtinggi3 = new FuzzyRuleAntecedent();
  payauANDtinggi3->joinWithAND(payau, tinggi);
  FuzzyRuleAntecedent *basa_6 = new FuzzyRuleAntecedent();
  basa_6->joinSingle(basa);
  FuzzyRuleAntecedent *payau_tinggiANDbasa = new FuzzyRuleAntecedent();
  payau_tinggiANDbasa->joinWithAND(payauANDtinggi3, basa_6);
  FuzzyRuleConsequent *then_inoff_AND_outon9 = new FuzzyRuleConsequent();
  then_inoff_AND_outon9->addOutput(in_off);
  then_inoff_AND_outon9->addOutput(out_on);
  FuzzyRule *fuzzyRule18 = new FuzzyRule(18, payau_tinggiANDbasa, then_inoff_AND_outon9);
  fuzzy->addFuzzyRule(fuzzyRule18);

  //asin
  FuzzyRuleAntecedent *asinANDrendah1 = new FuzzyRuleAntecedent();
  asinANDrendah1->joinWithAND(asn, rendah);
  FuzzyRuleAntecedent *asam_7 = new FuzzyRuleAntecedent();
  asam_7->joinSingle(asam);
  FuzzyRuleAntecedent *asin_rendahANDasam = new FuzzyRuleAntecedent();
  asin_rendahANDasam->joinWithAND(asinANDrendah1, asam_7);
  FuzzyRuleConsequent *then_inon_AND_outon1 = new FuzzyRuleConsequent();
  then_inon_AND_outon1->addOutput(in_on);
  then_inon_AND_outon1->addOutput(out_on);
  FuzzyRule *fuzzyRule19 = new FuzzyRule(19, asin_rendahANDasam, then_inon_AND_outon1);
  fuzzy->addFuzzyRule(fuzzyRule19);

  FuzzyRuleAntecedent *asinANDsedang1 = new FuzzyRuleAntecedent();
  asinANDsedang1->joinWithAND(asn, sedang);
  FuzzyRuleAntecedent *asam_8 = new FuzzyRuleAntecedent();
  asam_8->joinSingle(asam);
  FuzzyRuleAntecedent *asin_sedangANDasam = new FuzzyRuleAntecedent();
  asin_sedangANDasam->joinWithAND(asinANDsedang1, asam_8);
  FuzzyRuleConsequent *then_inon_AND_outon2 = new FuzzyRuleConsequent();
  then_inon_AND_outon2->addOutput(in_on);
  then_inon_AND_outon2->addOutput(out_on);
  FuzzyRule *fuzzyRule20 = new FuzzyRule(20, asin_sedangANDasam, then_inon_AND_outon2);
  fuzzy->addFuzzyRule(fuzzyRule20);

  FuzzyRuleAntecedent *asinANDtinggi1 = new FuzzyRuleAntecedent();
  asinANDtinggi1->joinWithAND(asn, tinggi);
  FuzzyRuleAntecedent *asam_9 = new FuzzyRuleAntecedent();
  asam_9->joinSingle(asam);
  FuzzyRuleAntecedent *asin_tinggiANDasam = new FuzzyRuleAntecedent();
  asin_tinggiANDasam->joinWithAND(asinANDtinggi1, asam_9);
  FuzzyRuleConsequent *then_inon_AND_outon3 = new FuzzyRuleConsequent();
  then_inon_AND_outon3->addOutput(in_on);
  then_inon_AND_outon3->addOutput(out_on);
  FuzzyRule *fuzzyRule21 = new FuzzyRule(21, asin_tinggiANDasam, then_inon_AND_outon3);
  fuzzy->addFuzzyRule(fuzzyRule21);

  FuzzyRuleAntecedent *asinANDrendah2 = new FuzzyRuleAntecedent();
  asinANDrendah2->joinWithAND(asn, rendah);
  FuzzyRuleAntecedent *netral_7 = new FuzzyRuleAntecedent();
  netral_7->joinSingle(netral);
  FuzzyRuleAntecedent *asin_rendahANDnetral = new FuzzyRuleAntecedent();
  asin_rendahANDnetral->joinWithAND(asinANDrendah2, netral_7);
  FuzzyRuleConsequent *then_inon_AND_outon4 = new FuzzyRuleConsequent();
  then_inon_AND_outon4->addOutput(in_on);
  then_inon_AND_outon4->addOutput(out_on);
  FuzzyRule *fuzzyRule22 = new FuzzyRule(22, asin_rendahANDnetral, then_inon_AND_outon4);
  fuzzy->addFuzzyRule(fuzzyRule22);

  FuzzyRuleAntecedent *asinANDsedang2 = new FuzzyRuleAntecedent();
  asinANDsedang2->joinWithAND(asn, sedang);
  FuzzyRuleAntecedent *netral_8 = new FuzzyRuleAntecedent();
  netral_8->joinSingle(netral);
  FuzzyRuleAntecedent *asin_sedangANDnetral = new FuzzyRuleAntecedent();
  asin_sedangANDnetral->joinWithAND(asinANDsedang2, netral_8);
  FuzzyRuleConsequent *then_inon_AND_outon5 = new FuzzyRuleConsequent();
  then_inon_AND_outon5->addOutput(in_on);
  then_inon_AND_outon5->addOutput(out_on);
  FuzzyRule *fuzzyRule23 = new FuzzyRule(23, asin_sedangANDnetral, then_inon_AND_outon5);
  fuzzy->addFuzzyRule(fuzzyRule23);

  FuzzyRuleAntecedent *asinANDtinggi2 = new FuzzyRuleAntecedent();
  asinANDtinggi2->joinWithAND(asn, tinggi);
  FuzzyRuleAntecedent *netral_9 = new FuzzyRuleAntecedent();
  netral_9->joinSingle(netral);
  FuzzyRuleAntecedent *asin_tinggiANDnetral = new FuzzyRuleAntecedent();
  asin_tinggiANDnetral->joinWithAND(asinANDtinggi2, netral_9);
  FuzzyRuleConsequent *then_inon_AND_outon6 = new FuzzyRuleConsequent();
  then_inon_AND_outon6->addOutput(in_on);
  then_inon_AND_outon6->addOutput(out_on);
  FuzzyRule *fuzzyRule24 = new FuzzyRule(24, asin_tinggiANDnetral, then_inon_AND_outon6);
  fuzzy->addFuzzyRule(fuzzyRule24);

  FuzzyRuleAntecedent *asinANDrendah3 = new FuzzyRuleAntecedent();
  asinANDrendah3->joinWithAND(asn, rendah);
  FuzzyRuleAntecedent *basa_7 = new FuzzyRuleAntecedent();
  basa_7->joinSingle(basa);
  FuzzyRuleAntecedent *asin_rendahANDbasa = new FuzzyRuleAntecedent();
  asin_rendahANDbasa->joinWithAND(asinANDrendah3, basa_7);
  FuzzyRuleConsequent *then_inon_AND_outon7 = new FuzzyRuleConsequent();
  then_inon_AND_outon7->addOutput(in_on);
  then_inon_AND_outon7->addOutput(out_on);
  FuzzyRule *fuzzyRule25 = new FuzzyRule(25, asin_rendahANDbasa, then_inon_AND_outon7);
  fuzzy->addFuzzyRule(fuzzyRule25);

  FuzzyRuleAntecedent *asinANDsedang3 = new FuzzyRuleAntecedent();
  asinANDsedang3->joinWithAND(asn, sedang);
  FuzzyRuleAntecedent *basa_8 = new FuzzyRuleAntecedent();
  basa_8->joinSingle(basa);
  FuzzyRuleAntecedent *asin_sedangANDbasa = new FuzzyRuleAntecedent();
  asin_sedangANDbasa->joinWithAND(asinANDsedang3, basa_8);
  FuzzyRuleConsequent *then_inon_AND_outon8 = new FuzzyRuleConsequent();
  then_inon_AND_outon8->addOutput(in_on);
  then_inon_AND_outon8->addOutput(out_on);
  FuzzyRule *fuzzyRule26 = new FuzzyRule(26, asin_sedangANDbasa, then_inon_AND_outon8);
  fuzzy->addFuzzyRule(fuzzyRule26);

  FuzzyRuleAntecedent *asinANDtinggi3 = new FuzzyRuleAntecedent();
  asinANDtinggi3->joinWithAND(asn, tinggi);
  FuzzyRuleAntecedent *basa_9 = new FuzzyRuleAntecedent();
  basa_9->joinSingle(basa);
  FuzzyRuleAntecedent *asin_tinggiANDbasa = new FuzzyRuleAntecedent();
  asin_tinggiANDbasa->joinWithAND(asinANDtinggi3, basa_9);
  FuzzyRuleConsequent *then_inon_AND_outon9 = new FuzzyRuleConsequent();
  then_inon_AND_outon9->addOutput(in_on);
  then_inon_AND_outon9->addOutput(out_on);
  FuzzyRule *fuzzyRule27 = new FuzzyRule(27, asin_tinggiANDbasa, then_inon_AND_outon9);
  fuzzy->addFuzzyRule(fuzzyRule27);
}

void setup()
{
  Serial.begin(9600);
  EEPROM.begin(32); //needed EEPROM.begin to store calibration k in eeprom
  pinMode(Tds_Pin, INPUT);
  pinMode(ph_Pin, INPUT);
  pinMode(pompainon, OUTPUT);
  pinMode(pompaouton, OUTPUT);
  sensors.begin();
  ec.begin();
  rule();

  //setting koneksi wifi
  WiFi.begin(ssid, password);

  //cek koneksi wifi
  while (WiFi.status() != WL_CONNECTED)
  {
    //terus mencoba koneksi
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Wifi Terhubung");
}

void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval)
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    MainFunction();
  }
}

void MainFunction()
{
  nilai_analog_PH = analogRead(ph_Pin);        // membaca analog ph
  TeganganPh = 3.3 / 4096.0 * nilai_analog_PH; // tegangan/pulse*analog ph
  PH_step = (PH4 - PH7) / 3;                   // pulse 4 - pulse 7 / 3
  Po = 7.00 + ((PH7 - TeganganPh) / PH_step);  //Po = 7.00 + ((teganganPh7 - TeganganPh) / PhStep);
  Serial.print("Nilai PH : ");
  Serial.println(Po, 2); // cetak nilai ph

  temperature = readTemperature(); // membaca temperatur
  Serial.print("temperature : ");
  Serial.println(temperature, 1); // cetak nilai temperatur

  nilai_analog_EC = analogRead(Tds_Pin);             // membaca analog tds
  ecValue = ec.readEC(nilai_analog_EC, temperature); // konversi analog ke ec
  Serial.print("EC:");
  Serial.print(ecValue, 1); // cetak nilai tds
  Serial.println(" ");

  //=====fuzzy=====
  float input1 = Po;
  float input2 = temperature;
  float input3 = ecValue;
  fuzzy->setInput(1, input1);
  fuzzy->setInput(2, input2);
  fuzzy->setInput(2, input3);
  fuzzy->fuzzify();
  // defuzzifikasi
  float output1 = fuzzy->defuzzify(1);
  float output2 = fuzzy->defuzzify(2);
  // Printing something
  Serial.print("Input");
  Serial.print("\n[ ph | temperatur | salinitas]: ");
  Serial.print("\n[ ");
  Serial.print(input1);
  Serial.print(" | ");
  Serial.print(input2);
  Serial.print(" | ");
  Serial.print(input3);
  Serial.print(" ]");
  Serial.print("\nOutput");
  Serial.print("\n[ Pompa In | Pompa Out ] ");
  Serial.print("\n[ Off | On | Off | On ]");
  Serial.print("\n[ ");
  Serial.print(in_off->getPertinence());
  Serial.print(" ");
  Serial.print(in_on->getPertinence());
  Serial.print(" | ");
  Serial.print(out_off->getPertinence());
  Serial.print(" ");
  Serial.print(out_on->getPertinence());
  Serial.print(" ] ");
  Serial.println();
  Serial.print("\nOutput Pompa In : ");
  Serial.print(output1);
  Serial.print("\nOutput Pompa Out : ");
  Serial.print(output2);
  if (output1 >= 100)
  {
    digitalWrite(pompainon, HIGH);
  }
  else if (output1 < 100)
  {
    digitalWrite(pompainon, LOW);
  }

  if (output2 >= 100)
  {
    digitalWrite(pompaouton, HIGH);
  }
  else if (output2 < 100)
  {
    digitalWrite(pompaouton, LOW);
  }

  WiFiClient client;
  if (!client.connect(host, 80))
  {
    Serial.println("Koneksi Gagal");
    return;
  }
  Serial.println("\n\n");

  //proses pengiriman data ke server
  String Link;
  HTTPClient http;
  Link = "http://" + String(host) + "/vanamei/kirimdata.php?ph=" + String(Po) + "&ppm=" + String(ecValue) + "&temp=" + String(temperature);
  //eksekusi link
  http.begin(Link);
  // mode GET
  http.GET();

  String respon = http.getString();
  Serial.println(respon);
  http.end();
}
