// =============================== Read the DS18b20 ===============================
float readDS() {
  byte i;
  //byte present;              // Quash a warning that 'present' is not used.
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;

  if (!ds.search(addr)) {
    ds.reset_search();
    delay(250);
    if (!ds.search(addr)) {       // start over. Search for the next device. If no more devices are found, false is returned.
      return (0);
    }
  }

  Serial.print(F("Device: 0x"));
  Serial.print(addr[0], HEX);
  Serial.print(F("  "));

  // the first ROM byte indicates which chip
  switch (addr[0])
  {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:          //ds18b20
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      Serial.println(F("Device is not a DS18x20 family device."));
      return (0);
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);                // Start conversion
  delay(1000);
  //present = ds.reset();         // Quash a warning that 'present' is not used.
  ds.reset();
  ds.select(addr);
  ds.write(0xBE);

  // Read Scratchpad
  for ( i = 0; i < 9; i++) {
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3;                // 9 bit resolution default
    if (data[7] == 0x10)
    {
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw & ~7;                    // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3;               // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1;               // 11 bit res, 375 ms
  }

  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  //Serial.print(F("  Temperature = "));
  //Serial.print(celsius);
  //Serial.print(F(" Celsius, "));
  //Serial.print(fahrenheit);
  //Serial.println(F(" Fahrenheit"));
  //Serial.println();
  return (fahrenheit);
  //##########
}
