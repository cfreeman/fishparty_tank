/*
 * Copyright (c) Clinton Freeman 2014
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */ 
#define __ASSERT_USE_STDERR
#include <assert.h>
#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

YunServer server;                                    // Listen on port 5555, the webserver on the Yun will forward all HTTP requests to us.
int NbTopsFan;                                       // The number of revolumeutions measured by the hall effect sensor.
unsigned long t;                                     // The last time that volumeume was measured.
volatile float volume;                               // The total volume measured by the flow sensor. Do not access directly,
                                                     // use getVolume to fetch the current dispensed volume.
static float total_volume = 1000.00;                 // The total volume of the drink dispenser in ml.
float target_full = 1.0;                             // The target percentage full that we want the tank to be.

/**
 * Callback method for pin2 interrupt. Updates the total volume that has been drained from
 * the drink dispenser. This method is called on each 'pulse' of the hall effect sensor in
 * the flow meter.
 */
void updatevolume () {
  unsigned long ct = millis();
  NbTopsFan++;  //Accumulate the pulses from the hall effect sesnors (rising edge).

  // Every ten spins of the flow sensor, calculate frequency and flow rate.
  if (NbTopsFan > 10) {
    float dV = (NbTopsFan / (0.073f * 60.0f)) * ((ct - t) / 1000.0f); // 73Q = 1L / Minute.
    volume += dV;

    NbTopsFan = 0;
    t = ct;
  }
}

/**
 * Returns the current volume (in ml) that has been poured by this beverage dispenser.
 */
float getVolume() {
  cli();
  float result = volume;
  sei();

  return result;
}

/**
 * Arduino initalisation.
 */
void setup() {
  pinMode(13,OUTPUT);
  digitalWrite(13, HIGH);                   // Engage the boot LED.
  Bridge.begin();                           // Begin the bridge between the two processors on the Yun.
  pinMode(2, INPUT);                        // Initializes digital pin 2 as an input
  attachInterrupt(1, updatevolume, RISING); // Sets pin 2 on the Arduino Yun as the interrupt.

  server.noListenOnLocalhost();             // Allow people to connect from the wider network.
  server.begin();                           // Warm up the HTTP server and redirect to here.
  digitalWrite(13, LOW);                    // Power down the boot LED.
}

/**
 * Main Arduino loop.
 */
void loop() {
  YunClient client = server.accept();
  if (client) {
    process(client);
    client.stop();                          // Close connection and free resources.
  }

  delay(50);
}

void process(YunClient client) {

  // Parse the incomming command.
  client.readStringUntil('/');
  String command = client.readStringUntil('/');
  command.trim();

  // If it is a drain command, parse the target volume.
  if (command == "drain") {
    target_full = client.parseFloat();
    Serial.print("Target Volume: ");
    Serial.println(target_full);
    client.print("Target Volume: ");
    client.println(target_full);
  }
}
