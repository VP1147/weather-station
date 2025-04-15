volatile byte revolutions;
float rpmilli;
float speed;

unsigned long timeold=0 ;

void setup()
{
    Serial.begin(9600);
    attachInterrupt(digitalPinToInterrupt(2), rpm_fun, RISING);

    revolutions = 0;
    rpmilli = 0;
    timeold = 0;
}

void loop()
{
    if (revolutions >= 1) {
        //Update RPM every 20 counts, increase this for better RPM resolution,
        //decrease for faster update

        // calculate the revolutions per milli(second)
        rpmilli = ((float)revolutions)/(millis()-timeold);

        timeold = millis();
        revolutions = 0;
        // Calculando a velocidade (m/s)
        speed = rpmilli * (2 * 3.1416 * 0.016) * 1000;

        Serial.print("RPM:");
        Serial.print(rpmilli * 60000 ,DEC);
        Serial.print("    Speed:");
        Serial.print(speed,DEC);
        Serial.println(" m/s");
    }
}

void rpm_fun()
{
    revolutions++;
}
