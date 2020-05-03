package tinyboy.core;

import java.util.Arrays;
import java.util.Iterator;
import java.util.function.Supplier;

import javr.core.AVR;
import javr.util.BitList;

/**
 * Represents a sequence of input values for the TinyBoy. The input sequence is
 * split into a series of "pulses" of a given (fixed) length. Each pulse is
 * either one of the buttons being pressed or nothing. For example, the sequence
 * <code>"LLRR_"</code> consists of two pulses with the left button down, two
 * with the right button down and one with nothing down.
 *
 * @author David J. Pearce
 *
 */
public class TinyBoyInputSequence implements Iterator<Boolean> {
	/**
	 * The number of inputs is determined by the number of buttons on the control
	 * pad.
	 */
	private final static int NUM_INPUTS = ControlPad.Button.values().length;
	/**
	 * The sequence of button push pulses.
	 */
	private final ControlPad.Button[] pulses;

	/**
	 * Clock determines where we are in the pulse sequence.
	 */
	private int clock = 0;

	public TinyBoyInputSequence(ControlPad.Button... pulses) {
		this.pulses = pulses;
	}

	/**
	 * Create a copy of a given input sequence.
	 *
	 * @param list
	 */
	public TinyBoyInputSequence(TinyBoyInputSequence list) {
		this.pulses = Arrays.copyOf(list.pulses, list.pulses.length);
	}

	/**
	 * Returns the number of pulses in this sequence.
	 * @return
	 */
	public int length() {
		return pulses.length;
	}

	@Override
	public boolean hasNext() {
		int n = clock / NUM_INPUTS;
		// NOTE: following is a little trick which basically allows the TinyBoy to
		// continue executing upto and including the next time the buttons are read.
		return n <= pulses.length;
	}

	/**
	 * Get next input in sequence.
	 */
	@Override
	public Boolean next() {
		int n = clock / NUM_INPUTS;
		int m = clock % NUM_INPUTS;
		// Increment clock
		clock = clock + 1;
		// Sanity check whether finished
		if(n >= pulses.length) {
			return false;
		} else {
			// Extract pulse
			ControlPad.Button b = pulses[n];
			// Determine whether high or low
			return (b != null) && (m == b.ordinal());
		}
	}

	/**
	 * Append a new pulse onto the end of this input sequence.
	 *
	 * @param pulse
	 * @return
	 */
	public TinyBoyInputSequence append(ControlPad.Button pulse) {
		final int len = pulses.length;
		TinyBoyInputSequence s = new TinyBoyInputSequence(Arrays.copyOf(pulses, len + 1));
		s.pulses[len] = pulse;
		return s;
	}

	/**
	 * Append a sequence of pulses onto the end of this input sequence.
	 *
	 * @param button
	 * @return
	 */
	public TinyBoyInputSequence append(ControlPad.Button[] nPulses) {
		final int n = pulses.length;
		final int m = nPulses.length;
		ControlPad.Button[] tmp = new ControlPad.Button[n+m];
		// Copy pulses over
		System.arraycopy(pulses, 0, tmp, 0, n);
		System.arraycopy(nPulses, 0, tmp, n, nPulses.length);
		// Create sequence of appropriate length.
		return new TinyBoyInputSequence(tmp);
	}


	@Override
	public String toString() {
		String r = "";
		for (int i = 0; i != pulses.length; ++i) {
			ControlPad.Button pulse = pulses[i];
			if(pulse == null) {
				r = r + "_";
			} else {
				r += pulse.toString().charAt(0);
			}
		}
		return r;
	}
}
