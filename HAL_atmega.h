
struct s_pin_def {
	uint8_t PINBIT;
	volatile uint8_t *DDREG;
	volatile uint8_t *PORTREG;
	volatile uint8_t *PINREG;
	uint8_t PCINR;
	uint8_t PCIBYTE;
	volatile uint8_t *PCICREG;
	volatile uint8_t *PCIMASK;
	uint8_t PCIEREG;
	uint8_t VEC;
};

static void set_pin_output(const s_pin_def *ptr_pin) {
	*ptr_pin->DDREG |= _BV(ptr_pin->PINBIT);
}

static void set_pin_high(const s_pin_def *ptr_pin) {
	*ptr_pin->PORTREG |= _BV(ptr_pin->PINBIT);
}

static void set_pin_low(const s_pin_def *ptr_pin) {
	*ptr_pin->PORTREG &= ~_BV(ptr_pin->PINBIT);
}