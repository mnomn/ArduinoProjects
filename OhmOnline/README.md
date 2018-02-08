#OhmOnline

Measure resistance and send it to a http URL.
Using ESP8266, which is the least suitable device for measuring analog stuff,
but very good at sending stuff to the internet.

Measure periodically.

To measure many voltages, do multiplexing by changing pins (Pa,Pb,Pc) from
INPUT (infinity resistance) to OUTPUT_LOW.

                      /-- Ra -- Pa
 3v --- Rref --- A0--+--- Rb -- Pb
                      \-- Rc -- Pc

That way you can measure three resistances Ra, Rb, Rc with A0
by changing the pins Pa, Pb and Pc.
