#include "AppSystem.h"

void error_notifier(int n) {
  
  if (n & LED0_MASK)  digitalWrite(LED0, HIGH);
  if (n & LED1_MASK)  digitalWrite(LED1, HIGH);
  if (n & LED2_MASK)  digitalWrite(LED2, HIGH);
  if (n & LED3_MASK)  digitalWrite(LED3, HIGH);
  delay(100);

  if (n & LED0_MASK)  digitalWrite(LED0, LOW);
  if (n & LED1_MASK)  digitalWrite(LED1, LOW);
  if (n & LED2_MASK)  digitalWrite(LED2, LOW);
  if (n & LED3_MASK)  digitalWrite(LED3, LOW);
  delay(100);

}
