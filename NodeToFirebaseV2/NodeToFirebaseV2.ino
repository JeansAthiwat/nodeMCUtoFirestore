
/**
 * SYNTAX:
 *
 * Firestore::Documents::createDocument(<AsyncClient>, <Firestore::Parent>, <documentPath>, <DocumentMask>, <Document>, <AsyncResult>);
 *
 * Firestore::Documents::createDocument(<AsyncClient>, <Firestore::Parent>, <collectionId>, <documentId>, <DocumentMask>, <Document>, <AsyncResult>);
 *
 * <AsyncClient> - The async client.
 * <Firestore::Parent> - The Firestore::Parent object included project Id and database Id in its constructor.
 * <documentPath> - The relative path of document to create in the collection.
 * <DocumentMask> - The fields to return. If not set, returns all fields. Use comma (,) to separate between the field names.
 * <collectionId> - The document id of document to be created.
 * <documentId> - The relative path of document collection id to create the document.
 * <Document> - The Firestore document.
 * <AsyncResult> - The async result (AsyncResult).
 *
 * The Firebase project Id should be only the name without the firebaseio.com.
 * The Firestore database id should be (default) or empty "".
 *
 * The complete usage guidelines, please visit https://github.com/mobizt/FirebaseClient
 */

#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_GIGA)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>) || defined(ARDUINO_NANO_RP2040_CONNECT)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>) || defined(ARDUINO_UNOWIFIR4)
#include <WiFiS3.h>
#elif __has_include(<WiFiC3.h>) || defined(ARDUINO_PORTENTA_C33)
#include <WiFiC3.h>
#elif __has_include(<WiFi.h>)
#include <WiFi.h>
#endif

#include <FirebaseClient.h>

#include <ESP8266WiFi.h>
#include <time.h>       // time() ctime()
#include <sys/time.h>   // struct timeval
#include <coredecls.h>  // settimeofday_cb()
#define TZ -8           // (utc+) TZ in hours
#define DST_MN 60       // use 60mn for summer time in some countries

#define TZ_MN ((TZ)*60)
#define TZ_SEC ((TZ)*3600)
#define DST_SEC ((DST_MN)*60)

timeval cbtime;  // time set in callback
bool cbtime_set = false;

void time_is_set(void) {
  gettimeofday(&cbtime, NULL);
  cbtime_set = true;
}


#define WIFI_SSID "Paopeaw"
#define WIFI_PASSWORD "12345677"

// The API key can be obtained from Firebase console > Project Overview > Project settings.
#define API_KEY "AIzaSyDKuF2rvUTePEPGIad4v3t68fzIRMX897E"

// User Email and password that already registerd or added in your project.
#define USER_EMAIL "paopeaw8246@gmail.com"
#define USER_PASSWORD "admin1234"
// #define DATABASE_URL "URL" // not used

#define FIREBASE_PROJECT_ID "embed-2024"

void printResult(AsyncResult& aResult);

String getTimestampString(uint64_t sec, uint32_t nano);

DefaultNetwork network;  // initilize with boolean parameter to enable/disable network reconnection

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);

FirebaseApp app;

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFiClientSecure.h>
WiFiClientSecure ssl_client;
#elif defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_UNOWIFIR4) || defined(ARDUINO_GIGA) || defined(ARDUINO_PORTENTA_C33) || defined(ARDUINO_NANO_RP2040_CONNECT)
#include <WiFiSSLClient.h>
WiFiSSLClient ssl_client;
#endif

using AsyncClient = AsyncClientClass;

AsyncClient aClient(ssl_client, getNetwork(network));

Firestore::Documents Docs;

AsyncResult aResult_no_callback;

bool taskCompleted = false;

int cnt = 0;

void setup() {

  Serial.begin(115200);

  settimeofday_cb(time_is_set);


  configTime(TZ_SEC, DST_SEC, "pool.ntp.org");



  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

  Serial.println("Initializing app...");

#if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040)
  ssl_client.setInsecure();
#if defined(ESP8266)
  ssl_client.setBufferSizes(4096, 1024);
#endif
#endif

  initializeApp(aClient, app, getAuth(user_auth), aResult_no_callback);

  app.getApp<Firestore::Documents>(Docs);
}

timeval tv;
timespec tp;
time_t now;
uint32_t now_ms, now_us;
uint32_t lastSend = 0;

#define PTM(w) \
  //  Serial.print(":" #w "="); \                             //Some extra prints I've cut out.. \
  //  Serial.print(tm->tm_##w);


void printTm(const char* what, const tm* tm) {
  //  Serial.print(what);
  PTM(isdst);
  PTM(yday);
  PTM(wday);
  PTM(year);
  PTM(mon);
  PTM(mday);
  PTM(hour);
  PTM(min);
  PTM(sec);
}

void loop() {
  // The async task handler should run inside the main loop
  // without blocking delay or bypassing with millis code blocks.

  // gettimeofday(&tv, nullptr);
  // now = time(nullptr);
  // now_ms = millis();
  // now_us = micros();

  // localtime / gmtime every second change
  // static time_t lastv = 0;
  // if (lastv != tv.tv_sec) {
  //   lastv = tv.tv_sec;
  //   printTm("localtime", localtime(&now));
  //   printTm("gmtime   ", gmtime(&now));
  // }

  // Serial.println(ctime(&now)); //This is where we actually print the time string.
  // Serial.println(now); //This is where we actually print the time string.

  // // simple drifting loop
  // delay(1000);

  app.loop();

  Docs.loop();

  // if (app.ready() && !taskCompleted) {
  if (app.ready() && millis()-lastSend >= 60000 || lastSend == 0) {
    lastSend = millis();
    // taskCompleted = true;

    // Note: If new document created under non-existent ancestor documents, that document will not appear in queries and snapshot
    // https://cloud.google.com/firestore/docs/using-console#non-existent_ancestor_documents.

    // We will create the document in the parent path "a0/b?
    // a0 is the collection id, b? is the document id in collection a0.

    String documentPath = "a0/b" + String(random(30000));

    // If the document path contains space e.g. "a b c/d e f"
    // It should encode the space as %20 then the path will be "a%20b%20c/d%20e%20f"

    // double
    Values::DoubleValue dblV(random(1, 500) / 100.0);
    Values::DoubleValue pressure(10);
    Values::DoubleValue humidity(10);
    Values::DoubleValue temperature(10);
    Values::IntegerValue z(10);
    Values::IntegerValue date(time(nullptr));

    // boolean
    Values::BooleanValue bolV(true);

    // integer
    Values::IntegerValue intV(random(500, 1000));

    // null
    Values::NullValue nullV;

    String doc_path = "projects/";
    doc_path += FIREBASE_PROJECT_ID;
    doc_path += "/databases/(default)/documents/coll_id/doc_id";  // coll_id and doc_id are your collection id and document id

    // reference
    Values::ReferenceValue refV(doc_path);

    // timestamp
    Values::TimestampValue tsV(getTimestampString(1712674441, 999999999));

    // bytes
    Values::BytesValue bytesV("aGVsbG8=");

    // string
    Values::StringValue strV("hello");

    // array
    Values::ArrayValue arrV(Values::StringValue("test"));
    arrV.add(Values::IntegerValue(20)).add(Values::BooleanValue(true));

    // map
    Values::MapValue mapV("name", Values::StringValue("wrench"));
    mapV.add("mass", Values::StringValue("1.3kg")).add("count", Values::IntegerValue(3));

    // lat long
    Values::GeoPointValue geoV(1.486284, 23.678198);

    Document<Values::Value> doc("pressure", Values::Value(pressure));
    // doc.add("myBool", Values::Value(bolV)).add("myInt", Values::Value(intV)).add("myNull", Values::Value(nullV));
    // doc.add("myRef", Values::Value(refV)).add("myTimestamp", Values::Value(tsV)).add("myBytes", Values::Value(bytesV));
    // doc.add("myString", Values::Value(strV)).add("myArr", Values::Value(arrV)).add("myMap", Values::Value(mapV));
    // doc.add("myGeo", Values::Value(geoV));
    doc.add("date", Values::Value(date));
    doc.add("humidity", Values::Value(humidity));
    doc.add("temperature", Values::Value(temperature));
    doc.add("z", Values::Value(z));

    // The value of Values::xxxValue, Values::Value and Document can be printed on Serial.

    Serial.println("Create document... ");

    Docs.createDocument(aClient, Firestore::Parent(FIREBASE_PROJECT_ID), documentPath, DocumentMask(), doc, aResult_no_callback);

  } 
  printResult(aResult_no_callback);
}

void printResult(AsyncResult& aResult) {
  if (aResult.isEvent()) {
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
  }

  if (aResult.isDebug()) {
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  }

  if (aResult.isError()) {
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  }

  if (aResult.available()) {
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
  
  }
}

String getTimestampString(uint64_t sec, uint32_t nano) {
  if (sec > 0x3afff4417f)
    sec = 0x3afff4417f;

  if (nano > 0x3b9ac9ff)
    nano = 0x3b9ac9ff;

  time_t now;
  struct tm ts;
  char buf[80];
  now = sec;
  ts = *localtime(&now);

  String format = "%Y-%m-%dT%H:%M:%S";

  if (nano > 0) {
    String fraction = String(double(nano) / 1000000000.0f, 9);
    fraction.remove(0, 1);
    format += fraction;
  }
  format += "Z";

  strftime(buf, sizeof(buf), format.c_str(), &ts);
  return buf;
}
