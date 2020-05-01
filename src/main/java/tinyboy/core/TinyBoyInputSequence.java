package tinyboy.core;

import java.util.Arrays;

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
public class TinyBoyInputSequence implements BitList {
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
	 * The width of a single pulse.
	 */
	private final int width;

	/**
	 * Create a new input sequence with a give number of pulses (i.e. button pushes)
	 * where each pulse has a given width (in cycles).
	 *
	 * @param size
	 *            The number of pulses in this sequence.
	 * @param width
	 *            The width of a pulse in this sequence.
	 */
	public TinyBoyInputSequence(int size, int width) {
		this.pulses = new ControlPad.Button[size];
		this.width = width;
	}

	/**
	 * Create a copy of a given input sequence.
	 *
	 * @param list
	 */
	public TinyBoyInputSequence(TinyBoyInputSequence list) {
		this.width = list.width;
		this.pulses = Arrays.copyOf(list.pulses, list.pulses.length);
	}

	private TinyBoyInputSequence(int width, ControlPad.Button[] pulses) {
		this.width = width;
		this.pulses = pulses;
	}

	@Override
	public int size() {
		return (pulses.length * width) * NUM_INPUTS;
	}

	/**
	 * Returns the number of pulses in this sequence.
	 * @return
	 */
	public int length() {
		return pulses.length;
	}

	@Override
	public boolean get(int ith) {
		int index = ith / (NUM_INPUTS * width);
		int input = ith % NUM_INPUTS;
		ControlPad.Button pulse = pulses[index];
		boolean r = pulse != null && pulse.ordinal() == input;
		return r;
	}

	@Override
	public void set(int ith, boolean value) {
		// Not supported!
		throw new UnsupportedOperationException();
	}

	/**
	 * Set a specific pulse to be either a button push, or no button pushed
	 * (<code>null</code>).
	 *
	 * @param pulse
	 * @param button
	 */
	public void setPulse(int pulse, ControlPad.Button button) {
		pulses[pulse] = button;
	}

	/**
	 * Append a new pulse onto the end of this input sequence.
	 *
	 * @param pulse
	 * @return
	 */
	public TinyBoyInputSequence append(ControlPad.Button pulse) {
		final int len = pulses.length;
		TinyBoyInputSequence s = new TinyBoyInputSequence(width, Arrays.copyOf(pulses, len + 1));
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
		// Create sequence of appropriate length.
		TinyBoyInputSequence s = new TinyBoyInputSequence(n + m, width);
		// Copy pulses over
		System.arraycopy(pulses, 0, s.pulses, 0, n);
		System.arraycopy(nPulses, 0, s.pulses, n, nPulses.length);
		// Done
		return s;
	}

	public String toBitString() {
		String r = "";
		for (int i = 0; i < size(); ++i) {
			if(get(i)) {
				r += "1";
			} else {
				r += "0";
			}
		}
		return r;
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
