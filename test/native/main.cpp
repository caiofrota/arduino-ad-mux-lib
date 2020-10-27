#include <unity.h>
#include <Mux.h>

using namespace admux;
using namespace fakeit;

void setUp(void) {
  ArduinoFakeReset();
}

void tearDown(void) {
}

void test_global(void) {
  TEST_ASSERT_FALSE(isDefined(UNDEFINED));
  TEST_ASSERT_FALSE(isDefined(-1));
  TEST_ASSERT_FALSE(isDefined(-100));
  TEST_ASSERT_TRUE(isDefined(0));
  TEST_ASSERT_TRUE(isDefined(1));
  TEST_ASSERT_TRUE(isDefined(100));
}

void test_Mux_channel(void) {
  When(Method(ArduinoFake(), pinMode)).AlwaysReturn();
  When(Method(ArduinoFake(), digitalWrite)).AlwaysReturn();

  Pinset channelPins(8, 9, 10);
  int8_t enabledPin = 20;
  int8_t writePin = 21;
  Mux mux(Pin(A0, Input, Analog), channelPins, enabledPin, writePin);

  TEST_ASSERT_EQUAL(1 << channelPins.size(), mux.channelCount());
  TEST_ASSERT_EQUAL(UNDEFINED, mux.channel());

  for (int i = 0; i < mux.channelCount(); i++) {
    // channel i selected.
    TEST_ASSERT_EQUAL(ERROR_SUCCESS, mux.channel(i));
    TEST_ASSERT_EQUAL(i, mux.channel());
    for (int p = 0; p < channelPins.size(); p++) {
      Unverified.Verify(
          Method(ArduinoFake(), digitalWrite).Using(channelPins[p],
              bitRead(i, p))).Once();
    }
    Unverified.Verify(Method(ArduinoFake(), digitalWrite).Using(writePin, LOW)).Once();
    Unverified.Verify(Method(ArduinoFake(), digitalWrite).Using(writePin, HIGH)).Once();
  }
  // out-of-range channel selection.
  TEST_ASSERT_EQUAL(ERROR_OUT_OF_RANGE, mux.channel(mux.channelCount()));
}

void test_Mux_constructor(void) {
  When(Method(ArduinoFake(), pinMode)).AlwaysReturn();
  When(Method(ArduinoFake(), digitalWrite)).AlwaysReturn();

  Mux mux(Pin(A0, Input, Analog), Pinset(8, 9, 10, 11));

  Pin signalPin = mux.signalPin();
  TEST_ASSERT_EQUAL(A0, signalPin.pin);
  TEST_ASSERT_EQUAL(Input, signalPin.mode);
  TEST_ASSERT_EQUAL(Analog, signalPin.type);

  Verify(Method(ArduinoFake(), pinMode).Using(8, OUTPUT)).Once();
  Verify(Method(ArduinoFake(), pinMode).Using(9, OUTPUT)).Once();
  Verify(Method(ArduinoFake(), pinMode).Using(10, OUTPUT)).Once();
  Verify(Method(ArduinoFake(), pinMode).Using(11, OUTPUT)).Once();

  TEST_ASSERT_EQUAL(16, mux.channelCount());
}

void test_Mux_enabled(void) {
  When(Method(ArduinoFake(), pinMode)).AlwaysReturn();
  When(Method(ArduinoFake(), digitalWrite)).AlwaysReturn();

  // enable-pin undefined.
  Mux mux(Pin(A0, Input, Analog), Pinset(8, 9, 10, 11));
  TEST_ASSERT_EQUAL(ERROR_UNDEFINED_PIN, mux.enabled(true));

  // enable-pin defined:
  int8_t enablePin = 12;
  // - set to true
  mux = Mux(Pin(A0, Input, Analog), Pinset(8, 9, 10, 11), enablePin);
  TEST_ASSERT_EQUAL(ERROR_SUCCESS, mux.enabled(true));
  Verify(Method(ArduinoFake(), digitalWrite).Using(enablePin, LOW)).Once();
  Verify(Method(ArduinoFake(), digitalWrite).Using(enablePin, HIGH)).Never();

  // - set to false
  TEST_ASSERT_EQUAL(ERROR_SUCCESS, mux.enabled(false));
  Unverified.Verify(Method(ArduinoFake(), digitalWrite).Using(enablePin, HIGH)).Once();
  Unverified.Verify(Method(ArduinoFake(), digitalWrite).Using(enablePin, LOW)).Never();
}

void test_Mux_read(void) {
  When(Method(ArduinoFake(), pinMode)).AlwaysReturn();
  When(Method(ArduinoFake(), digitalWrite)).AlwaysReturn();
  When(Method(ArduinoFake(), analogRead)).AlwaysReturn();
  When(Method(ArduinoFake(), digitalRead)).AlwaysReturn();

  // Analog input.
  Pin signalPin(A0, Input, Analog);
  Pinset channelPins(8, 9, 10);
  int8_t enabledPin = 20;
  int8_t writePin = 21;
  Mux mux(signalPin, channelPins, enabledPin, writePin);

  TEST_ASSERT_EQUAL(ERROR_OUT_OF_RANGE, mux.read(mux.channelCount()));

  TEST_ASSERT_EQUAL(ERROR_SUCCESS, mux.read(mux.channelCount() - 1));
  Unverified.Verify(Method(ArduinoFake(), analogRead).Using(signalPin.pin)).Once();
  Unverified.Verify(Method(ArduinoFake(), digitalRead).Using(signalPin.pin)).Never();

  // Digital input.
  mux = Mux(signalPin = Pin(3, Input, Digital), channelPins, enabledPin, writePin);

  TEST_ASSERT_EQUAL(ERROR_SUCCESS, mux.read(mux.channelCount() - 1));
  Unverified.Verify(Method(ArduinoFake(), analogRead).Using(signalPin.pin)).Never();
  Unverified.Verify(Method(ArduinoFake(), digitalRead).Using(signalPin.pin)).Once();
}

void test_Mux_write(void) {
  When(Method(ArduinoFake(), pinMode)).AlwaysReturn();
  When(Method(ArduinoFake(), digitalWrite)).AlwaysReturn();
  When(Method(ArduinoFake(), analogWrite)).AlwaysReturn();

  uint8_t data = 128;

  // Analog input.
  Pin signalPin(A0, Input, Analog);
  Pinset channelPins(8, 9, 10);
  int8_t enabledPin = 20;
  int8_t writePin = 21;
  Mux mux(signalPin, channelPins, enabledPin, writePin);

  TEST_ASSERT_EQUAL(ERROR_WRONG_SIGNAL_MODE, mux.write(data, mux.channelCount()));

  // Analog output.
  mux = Mux(signalPin = Pin(3, Output, Analog), channelPins, enabledPin, writePin);

  TEST_ASSERT_EQUAL(ERROR_OUT_OF_RANGE, mux.write(data, mux.channelCount()));

  TEST_ASSERT_EQUAL(ERROR_SUCCESS, mux.write(data, mux.channelCount() - 1));
  Unverified.Verify(Method(ArduinoFake(), analogWrite).Using(signalPin.pin, data)).Once();
  Unverified.Verify(Method(ArduinoFake(), digitalWrite).Using(signalPin.pin, data)).Never();

  // Digital output.
  mux = Mux(signalPin = Pin(3, Output, Digital), channelPins, enabledPin, writePin);

  TEST_ASSERT_EQUAL(ERROR_SUCCESS, mux.write(data, mux.channelCount() - 1));
  Unverified.Verify(Method(ArduinoFake(), analogWrite).Using(signalPin.pin, data)).Never();
  Unverified.Verify(Method(ArduinoFake(), digitalWrite).Using(signalPin.pin, data)).Once();
}

void test_Pin(void) {
  Pin pin(A0, Input, Analog);

  TEST_ASSERT_EQUAL(A0, pin.pin);
  TEST_ASSERT_EQUAL(Input, pin.mode);
  TEST_ASSERT_EQUAL(Analog, pin.type);
}

void test_Pinset(void) {
  Pinset pinset(8, 9, 12, 14);

  TEST_ASSERT_EQUAL(4, pinset.size());
  TEST_ASSERT_EQUAL(8, pinset[0]);
  TEST_ASSERT_EQUAL(9, pinset[1]);
  TEST_ASSERT_EQUAL(12, pinset[2]);
  TEST_ASSERT_EQUAL(14, pinset[3]);
  TEST_ASSERT_EQUAL(UNDEFINED, pinset[4]);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();

  RUN_TEST(test_global);

  RUN_TEST(test_Pin);

  RUN_TEST(test_Pinset);

  RUN_TEST(test_Mux_constructor);
  RUN_TEST(test_Mux_channel);
  RUN_TEST(test_Mux_enabled);
  RUN_TEST(test_Mux_read);
  RUN_TEST(test_Mux_write);

  return UNITY_END();
}
