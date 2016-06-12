#include <stdio.h>

extern "C" {
extern void setup();
extern void loop();
}

__attribute((weak))
void setup() {
	// do nothing
}

__attribute((weak))
void loop() {
	// do nothing
}

__attribute ((weak))
int main(int argc, char** argv)
{
	setup();
	loop();
}

