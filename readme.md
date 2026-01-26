tldr: a rendszer a BMP280 és SGP30 I2C-szenzorokat, valamint a DHT11 digitális szenzort kezeli. az adatokat microSD-re menti az SPI1-en, míg az SPI0-on a 868 MHz-es LoRa továbbítja. az ESP32-S3-CAM a saját beépített SD kártyájára ment. ha egy szenzor nem elérhető vagy hibásan olvas, az adott adat nem kerül rádióra. a rádión küldött sor fix hosszúságú mezőkkel, vesszővel elválasztva, üres mező esetén a vessző megmarad, pl. 2310,100212,,,2412,19,2500,6230, ahol az értékek integer formátumban vannak (hőmérséklet ×100, légnyomás ×100, eCO2 ppm, TVOC ppb, DHT hőmérséklet ×100, páratartalom ×100).

---

# TODO
###### jelen lista a v1-hez készült, azóta elavult.
- [ ] BMP280 szenzor bármely kábelének kontakthibája esetén lekezelni (akár megszűnik a táp, vagy az adatkábel elmozdult, stb)
- [ ] OV7670 alternatíva keresése (nincs FIFO támogatás, és driver, felejtős)
- [ ] SD-kártyás logolás
- [ ] plusz szenzor beszerzése, szoftver megírása hozzá
- [ ] LoRa modulhoz megírni, megtervezni a logikáját és adatstruktúráját (tesztelés alatt)
- [ ] nem sleep_ms blokkoló időzítést használni
- [ ] kijelzőt mire használjuk?
