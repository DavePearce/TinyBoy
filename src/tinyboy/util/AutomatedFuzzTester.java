package tinyboy.util;

import java.io.FileReader;
import java.io.IOException;
import java.util.Arrays;
import java.util.BitSet;
import java.util.Iterator;
import java.util.List;

import javaavr.io.HexFile;
import javaavr.util.BitList;
import tinyboy.core.ButtonPad.Button;
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
	public CoverageAnalysis run(int maxSteps) {
		CoverageAnalysis analysis = new CoverageAnalysis(firmware);
		for (int i = 0; i != maxSteps; ++i) {
			//
			if (generator.hasNext()) {
				BitList inputs = generator.next();
				BitSet covered = fuzzTest(inputs);
				analysis.record(covered);
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
	private BitSet fuzzTest(BitList input) {
		tinyBoy.reset();
		tinyBoy.upload(firmware);
		// Keep going until input is exhausted
		for (int i = 0; (i + 3) < input.size(); i = i + 4) {
			// Read the next set of inputs
			boolean up = input.get(i);
			boolean right = input.get(i+1);
			boolean down = input.get(i+2);
			boolean left = input.get(i+3);
			// Apply the next set of inputs
			tinyBoy.set(Button.UP,up);
			tinyBoy.set(Button.DOWN,down);
			tinyBoy.set(Button.LEFT,left);
			tinyBoy.set(Button.RIGHT,right);
			// Finally, clock the tiny boy
			tinyBoy.clock();
		}
		// FIXME: this should be replaced with an instrumentation adaptor.
		return tinyBoy.getCoverage();
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
		if(inputs.hasNext()) {
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
	public interface InputGenerator {
		/**
		 * Check whether there are any more inputs to generate.
		 *
		 * @return
		 */
		public boolean hasNext();

		/**
		 * Get the next generated input.
		 *
		 * @return
		 */
		public BitList next();
	}

	public static void main(String[] args) throws IOException {
		TinyBoyEmulator tbem = new TinyBoyEmulator();
		HexFile.Reader hfr = new HexFile.Reader(new FileReader("tetris.hex"));
		HexFile firmware = hfr.readAll();
		InputGenerator generator = new InputGenerator() {
			private boolean next = true;
			@Override
			public boolean hasNext() {
				return next;
			}

			@Override
			public BitList next() {
				next = false;
				return new BitList() {

					@Override
					public int size() {
						return 100000;
					}

					@Override
					public boolean get(int ith) {
						// TODO Auto-generated method stub
						return false;
					}

					@Override
					public void set(int ith, boolean value) {
						// TODO Auto-generated method stub
					}
				};
			}

		};
		// Create the tester
		AutomatedFuzzTester tester = new AutomatedFuzzTester(tbem,firmware,generator);
		CoverageAnalysis coverage = tester.run(10);
		System.out.println("Coverage: " + coverage.getInstructionCoverage());
	}
}
