// gfvalvo's flash string helper code: https://forum.arduino.cc/index.php?topic=533118.msg3634809#msg3634809
void SerialPrint(const char *);
void SerialPrint(const __FlashStringHelper *);
void SerialPrintln(const char *);
void SerialPrintln(const __FlashStringHelper *);
void DoSerialPrint(char (*)(const char *), const char *, bool newLine = false);

void SerialPrint(const char *line) {
  DoSerialPrint([](const char *ptr) {return *ptr;}, line);
}

void SerialPrint(const __FlashStringHelper *line) {
  DoSerialPrint([](const char *ptr) {return (char) pgm_read_byte_near(ptr);},(const char*) line);
}

void SerialPrintln(const char *line) {
  DoSerialPrint([](const char *ptr) {return *ptr;}, line, true);
}

void SerialPrintln(const __FlashStringHelper *line) {
  DoSerialPrint([](const char *ptr) {return (char) pgm_read_byte_near(ptr);}, (const char*) line, true);
}

void DoSerialPrint(char (*funct)(const char *), const char *string, bool newLine) {
  char ch;
  while ((ch = funct(string++))) Serial.print(ch);
  if (newLine) Serial.println();
}