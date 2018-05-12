package tinyboy.core;

import javaavr.core.AvrPeripheral;
import javaavr.core.Wire;
import javaavr.util.IdealWire;

/**
 * Represents the input pad on the TinyBoy console.
 *
 * @author David J. Pearce
 *
 */
public class ButtonPad implements AvrPeripheral {
	public enum Button {
		UP, DOWN, LEFT, RIGHT
	}
	//
	private final IdealWire[] wires;

	public ButtonPad() {
		this.wires = new IdealWire[4];
		for (Button b : Button.values()) {
			wires[b.ordinal()] = new IdealWire(b.name());
		}
	}

	@Override
	public Wire[] getWires() {
		return wires;
	}

	@Override
	public void clock() {
		for(int i=0;i!=wires.length;++i) {
			wires[i].clock();
		}
	}

	/**
	 * Set the value of a given button on the TinyBoy. This button will remain in
	 * this state until it is changed again.
	 *
	 * @param button
	 * @param value
	 */
	public void set(Button button, boolean value) {
		wires[button.ordinal()].write(value);
	}
}
