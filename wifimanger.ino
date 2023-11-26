void wifiConnect() {

  //wifiManager.resetSettings();
  
  if (!wifiManager.autoConnect(wifimanager_ssid, wifimanager_password))
  {
    Serial.println("Failed to connect, we should reset and see if it connects");

    delay(3000);
    ESP.restart();
    delay(5000);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());  
}
