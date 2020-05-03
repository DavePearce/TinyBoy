package tinyboy.util;

import java.io.FileReader;
import java.io.IOException;
import java.util.Arrays;
import java.util.BitSet;
import java.util.Iterator;
import java.util.List;
import java.util.function.Supplier;

import javr.core.AVR;
import javr.core.AVR.HaltedException;
import javr.core.AvrConfiguration;
import javr.core.Wire;
import javr.io.HexFile;
import javr.memory.instruments.ReadWriteInstrument;
import javr.util.BitList;
import javr.util.IdealWire;
import javrsim.peripherals.JPeripheral;
import tinyboy.core.ControlPad.Button;
import tinyboy.views.TinyBoyPeripheral;
import tinyboy.core.ControlPad;
import tinyboy.core.TinyBoyEmulator;

/**
 * Provides a simple framework for generating tests and fuzzing a given firmware
 * for the TinyBoy.
 *
 * @author David J. Pearce
 *
 */
public class AutomatedTester<T extends Supplier<Boolean>> {
	private final ExtendedTinyBoyEmulator tinyBoy;
	private final HexFile firmware;
	private final InputGenerator<T> generator;

	public AutomatedTester(HexFile firmware, InputGenerator<T> generator) {
		this.tinyBoy = createTinyBoy();
		this.firmware = firmware;
		this.generator = generator;
	}

	public TinyBoyEmulator getTinyBoy() {
		return tinyBoy;
	}

	/**
	 * Run the fuzzer for a given number of steps producing a coverage report. This
	 * will run the tool for a maximum number of iterations, where each test is run
	 * for a maximum number of cycles against a given coverage target. If the target
	 * is achieved, the test stops immediately.
	 *
	 * @param iterations
	 *            The number of iterations of fuzzing to perform. Each iteration
	 *            consists of generating a new input based on previous inputs seen.
	 * @param cycles
	 *            The maximum number of cycles to run a test for. This is
	 *            effectively a timeout to ensure tests don't go on forever.
	 * @param target
	 *            The target coverage and, once achieved, testing will stop.
	 * @return
	 */
	public CoverageAnalysis run(int iterations, int cycles, double target) {
		CoverageAnalysis analysis = new CoverageAnalysis(firmware);
		int i = 0;
		while (analysis.getBranchCoverage() < target && i < iterations) {
			T inputs = generator.generate();
			//
			if (inputs != null) {
				// Perform the test
				Result r = fuzzTest(inputs, cycles);
				BitSet covered = r.getCoverage();
				// Insure only instructions returned.
				covered.and(analysis.getReachableInstructions());
				// Register the output with the generator so that it can refine its strategy
				// based on this.
				generator.record(inputs, r.getCoverage(), r.getState());
				// Record the output with the coverage analysis so that we can subsequently
				// compute coverage data.
				analysis.record(covered);
			} else {
				break;
			}
			i = i + 1;
		}
		return analysis;
	}

	/**
	 * Actually fuzz test the TinyBoy with a given sequence of input values.
	 *
	 * @param input
	 * @return
	 * @throws HaltedException
	 */
	private Result fuzzTest(Supplier<Boolean> input, int cycles) {
		int numButtons = Button.values().length;
		// Reset the tiny boy
		tinyBoy.reset();
		tinyBoy.upload(firmware);
		tinyBoy.bind(input);
		// Attach the instrumentation
		ReadWriteInstrument instrument = new ReadWriteInstrument();
		tinyBoy.getAVR().getCode().register(instrument);
		// Keep going until input is exhausted
		//int max = Math.min(cycles, input.size() / numButtons );
		try {
			for (int i = 0; i < cycles; i = i + 1) {
//				int ith = i * numButtons;
//				// Read the next set of inputs
//				boolean up = input.get(ith + Button.UP.ordinal());
//				boolean right = input.get(ith + Button.RIGHT.ordinal());
//				boolean down = input.get(ith + Button.DOWN.ordinal());
//				boolean left = input.get(ith + Button.LEFT.ordinal());
//				// Apply the next set of inputs
//				tinyBoy.setButtonState(Button.UP, up);
//				tinyBoy.setButtonState(Button.DOWN, down);
//				tinyBoy.setButtonState(Button.LEFT, left);
//				tinyBoy.setButtonState(Button.RIGHT, right);
				// Finally, clock the tiny boy
				tinyBoy.clock();
			}
		} catch (HaltedException e) {
		}
		// Remove the instrumentation
		tinyBoy.getAVR().getCode().unregister(instrument);
		byte[] data = toByteArray(tinyBoy.getAVR().getData());
		// Extract the coverage data
		return new Result(instrument.getReads(),data);
	}

	/**
	 * Create a TinyBoy emulator which has an optional graphical display.
	 *
	 * @return
	 */
	private ExtendedTinyBoyEmulator createTinyBoy() {
		SymbolicPullWire[] wires = new SymbolicPullWire[4];
		wires[ControlPad.Button.UP.ordinal()] = new SymbolicPullWire("PB1", "MISO", "DO", "AIN1", "OC0B", "OC1A", "PCINT1");
		wires[ControlPad.Button.DOWN.ordinal()] = new SymbolicPullWire("PB3", "PCINT3", "XTAL1", "CLK1", "!OC1B", "ADC3");
		wires[ControlPad.Button.LEFT.ordinal()] = new SymbolicPullWire("PB4", "PCINT4", "XTAL2", "CLK0", "OC1B", "ADC2");
		wires[ControlPad.Button.RIGHT.ordinal()] = new SymbolicPullWire("PB5", "PCINT5", "!RESET", "ADC0", "dW");

		return new ExtendedTinyBoyEmulator(wires);
	}

	private static class ExtendedTinyBoyEmulator extends TinyBoyEmulator {
		private final SymbolicPullWire[] wires;
		private final JPeripheral view = new TinyBoyPeripheral(this);

		public ExtendedTinyBoyEmulator(SymbolicPullWire[] wires) {
			super(labels -> getWire(wires,labels));
			this.wires = wires;
		}

		public void bind(Supplier<Boolean> input) {
			for(int i=0;i!=wires.length;++i) {
				wires[i].bind(input);
			}
		}

		@Override
		public void clock() throws HaltedException {
			final AVR mcu = this.getAVR();
			// Clock peripheral first
			view.clock();
			// Clock AVR second
			mcu.clock();
		}

		@Override
		public void destroy() {
			view.setVisible(false);
			view.dispose();
		}

		private static Wire getWire(SymbolicPullWire[] wires, String[] labels) {
			switch(labels[0]) {
			case "PB1": // Up
				return wires[Button.UP.ordinal()];
			case "PB3": // Down
				return wires[Button.DOWN.ordinal()];
			case "PB4": // Left
				return wires[Button.LEFT.ordinal()];
			case "PB5": // Right
				return wires[Button.RIGHT.ordinal()];
			default:
				return new IdealWire(labels);
			}
		}

		@Override
		public boolean getButtonState(ControlPad.Button button) {
			// NOTE: this override is necessary to prevent the GUI from generating read
			// requests on the I/O pins representing the buttons which, in turn, interfer
			// with our input streams.
			return false;
		}
	}


	private static class SymbolicPullWire implements Wire {
		private final String[] labels;
		private Supplier<Boolean> input;

		public SymbolicPullWire(String... labels) {
			this.labels = labels;
			// Default input
			this.input = () -> false;
		}

		public void bind(Supplier<Boolean> input) {
			this.input = input;
		}
		@Override
		public String[] getLabels() {
			return labels;
		}

		@Override
		public boolean hasLabel(String label) {
			for(int i=0;i!=labels.length;++i) {
				if(labels[i].equals(label)) {
					return true;
				}
			}
			return false;
		}

		@Override
		public boolean read() {
			return input.get();
		}

		@Override
		public boolean write(boolean value) {
			return false;
		}

		@Override
		public boolean isRising() {
			return false;
		}

		@Override
		public boolean clock() {
			// TODO Auto-generated method stub
			return false;
		}

		@Override
		public void reset() {
			// no-op
		}

	}

	/**
	 * Read the contents of memory into a byte array.
	 *
	 * @param memory
	 * @return
	 */
	private byte[] toByteArray(AVR.Memory memory) {
		byte[] bytes = new byte[memory.size()];
		for(int i=0;i!=bytes.length;++i) {
			bytes[i] = memory.peek(i);
		}
		return bytes;
	}

	/**
	 * Attempt to read the next input from the inputs sequence. If no such input
	 * exists, return a default value instead.
	 *
	 * @param inputs
	 *            The sequence of inputs being read.
	 * @return
	 */
	private boolean readNext(Iterator<Boolean> inputs) {
		if (inputs.hasNext()) {
			return inputs.next();
		} else {
			return false;
		}
	}

	/**
	 * An input generator is responsible for generating inputs which are fed into
	 * the testing tool.
	 *
	 * @author David J. Pearce
	 *
	 */
	public interface InputGenerator<T extends Supplier<Boolean>> {
		/**
		 * Get the next generated input.
		 *
		 * @return
		 */
		public T generate();

		/**
		 * Record the result of a given test. That is, for a generated input, record the
		 * actual set of covered instructions along with the final state.
		 *
		 * @param input
		 * @param output
		 */
		public void record(T input, BitSet output, byte[] state);
	}

	/**
	 * Represents the result from a fuzzing run.
	 *
	 * @author djp
	 *
	 */
	public class Result {
		/**
		 * Instructions covered by this run.
		 */
		private final BitSet coverage;
		/**
		 * State of machine memory at end of run.
		 */
		private final byte[] state;

		public Result(BitSet coverage, byte[] state) {
			this.coverage = coverage;
			this.state = state;
		}

		public BitSet getCoverage() {
			return coverage;
		}

		public byte[] getState() {
			return state;
		}
	}
}
