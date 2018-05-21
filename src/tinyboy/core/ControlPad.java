package tinyboy.core;

import javr.core.AvrPeripheral;
import javr.core.Wire;
import javr.util.IdealWire;

/**
 * Represents the input pad on the TinyBoy console. That consists of four
 * directional buttons.
 *
 * @author David J. Pearce
 *
 */
public class ControlPad implements AvrPeripheral {
	public enum Button {
		UP, DOWN, LEFT, RIGHT
	}
	//
	private final Wire[] wires;

	public ControlPad(Wire up, Wire down, Wire left, Wire right) {
		this.wires = new IdealWire[4];
		this.wires[Button.UP.ordinal()] = up;
		this.wires[Button.DOWN.ordinal()] = down;
		this.wires[Button.LEFT.ordinal()] = left;
		this.wires[Button.RIGHT.ordinal()] = right;
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

	@Override
	public void reset() {

	}

	/**
	 * Read the current state of a given button.
	 *
	 * @param button
	 * @return
	 */
	public boolean getState(Button button) {
		return wires[button.ordinal()].read();
	}

	/**
	 * Set the value of a given button on the TinyBoy. This button will remain in
	 * this state until it is changed again.
	 *
	 * @param button
	 * @param value
	 */
	public void setState(Button button, boolean value) {
		wires[button.ordinal()].write(value);
	}
}
