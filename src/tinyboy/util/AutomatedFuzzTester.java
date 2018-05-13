package tinyboy.util;

import java.io.FileReader;
import java.io.IOException;
import java.util.Arrays;
import java.util.BitSet;
import java.util.Iterator;
import java.util.List;

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
public class AutomatedFuzzTester {
	private final TinyBoyEmulator tinyBoy;
	private final HexFile firmware;
	private final InputGenerator generator;

	public AutomatedFuzzTester(TinyBoyEmulator tinyBoy, HexFile firmware, InputGenerator generator) {
		this.tinyBoy = tinyBoy;
		this.firmware = firmware;
		this.generator = generator;
	}

	/**
	 * Run the fuzzer for a given number of steps producing a coverage report.
	 *
	 * @return
	 */
	public CoverageAnalysis run(int maxSteps, int cycles) {
		CoverageAnalysis analysis = new CoverageAnalysis(firmware);
		for (int i = 0; i != maxSteps; ++i) {
			//
			BitList inputs = generator.generate();
			if (inputs != null) {
				// Perform the test
				BitSet covered = fuzzTest(inputs, cycles);
				// Register the output with the generator so that it can refine its strategy
				// based on this.
				generator.record(inputs, covered);
				// Record the output with the coverage analysis so that we can subsequently
				// compute coverage data.
				System.out.println("REGISTERED: " + covered.cardinality() + " size");
				analysis.record(covered);
			} else {
				break;
			}
		}
		return analysis;
	}

	/**
	 * Actually fuzz test the TinyBoy with a given sequence of input values.
	 *
	 * @param input
	 * @return
	 */
	private BitSet fuzzTest(BitList input, int cycles) {
		// Reset the tiny boy
		tinyBoy.reset();
		tinyBoy.upload(firmware);
		// Attach the instrumentation
		ReadWriteInstrument instrument = new ReadWriteInstrument();
		tinyBoy.getAVR().getCode().register(instrument);
		// Keep going until input is exhausted
		int max = Math.min(cycles, input.size() / 4);
		//
		for (int i = 0; i < max; i = i + 1) {
			int ith = i << 2;
			// Read the next set of inputs
			boolean up = input.get(ith);
			boolean right = input.get(ith + 1);
			boolean down = input.get(ith + 2);
			boolean left = input.get(ith + 3);
			// Apply the next set of inputs
			tinyBoy.setButtonState(Button.UP, up);
			tinyBoy.setButtonState(Button.DOWN, down);
			tinyBoy.setButtonState(Button.LEFT, left);
			tinyBoy.setButtonState(Button.RIGHT, right);
			// Finally, clock the tiny boy
			tinyBoy.clock();
		}
		// Remove the instrumentation
		tinyBoy.getAVR().getCode().unregister(instrument);
		// Extract the coverage data
		return instrument.getReads();
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
		 * actual output.
		 *
		 * @param input
		 * @param output
		 */
		public void record(T input, BitSet output);
	}
}
