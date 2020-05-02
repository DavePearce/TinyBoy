package tinyboy.util;

import java.io.FileReader;
import java.io.IOException;
import java.util.Arrays;
import java.util.BitSet;
import java.util.Iterator;
import java.util.List;

import javr.core.AVR;
import javr.core.AVR.HaltedException;
import javr.io.HexFile;
import javr.memory.instruments.ReadWriteInstrument;
import javr.util.BitList;
import tinyboy.core.ControlPad.Button;
import tinyboy.core.TinyBoyEmulator;

/**
 * Provides a simple framework for generating tests and fuzzing a given firmware
 * for the TinyBoy.
 *
 * @author David J. Pearce
 *
 */
public class AutomatedTester {
	private final TinyBoyEmulator tinyBoy;
	private final HexFile firmware;
	private final InputGenerator generator;

	public AutomatedTester(TinyBoyEmulator tinyBoy, HexFile firmware, InputGenerator generator) {
		this.tinyBoy = tinyBoy;
		this.firmware = firmware;
		this.generator = generator;
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
			BitList inputs = generator.generate();
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
	private Result fuzzTest(BitList input, int cycles) {
		int numButtons = Button.values().length;
		// Reset the tiny boy
		tinyBoy.reset();
		tinyBoy.upload(firmware);
		// Attach the instrumentation
		ReadWriteInstrument instrument = new ReadWriteInstrument();
		tinyBoy.getAVR().getCode().register(instrument);
		// Keep going until input is exhausted
		int max = Math.min(cycles, input.size() / numButtons );
		try {
			for (int i = 0; i < max; i = i + 1) {
				int ith = i * numButtons;
				// Read the next set of inputs
				boolean up = input.get(ith + Button.UP.ordinal());
				boolean right = input.get(ith + Button.RIGHT.ordinal());
				boolean down = input.get(ith + Button.DOWN.ordinal());
				boolean left = input.get(ith + Button.LEFT.ordinal());
				// Apply the next set of inputs
				tinyBoy.setButtonState(Button.UP, up);
				tinyBoy.setButtonState(Button.DOWN, down);
				tinyBoy.setButtonState(Button.LEFT, left);
				tinyBoy.setButtonState(Button.RIGHT, right);
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
	public interface InputGenerator<T extends BitList> {
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
