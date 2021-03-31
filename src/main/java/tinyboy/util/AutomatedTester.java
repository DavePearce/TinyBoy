package tinyboy.util;

import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.BitSet;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.function.Supplier;

import org.eclipse.jdt.annotation.NonNull;
import org.eclipse.jdt.annotation.Nullable;

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
public class AutomatedTester<T extends Iterator<Boolean>> {
	/**
	 * Construct a thread pool to use for parallel processing.
	 */
	private static final ExecutorService executor = Executors.newCachedThreadPool();

	/**
	 * TinyBoy instance being fuzzed
	 */
	private final ExtendedTinyBoyEmulator[] tinyBoys;
	/**
	 * Firmware image to be fuzzed.
	 */
	private final HexFile firmware;
	/**
	 * Input generate to drive fuzzing.
	 */
	private final InputGenerator<T> generator;
	/**
	 * Number of threads to use for parallel processing.
	 */
	private final int nthreads;
	/**
	 * Batch size to use for each thread.
	 */
	private final int batchSize;

	public AutomatedTester(HexFile firmware, InputGenerator<T> generator, boolean gui, int nthreads, int batchSize) {
		this.firmware = firmware;
		this.generator = generator;
		this.tinyBoys = new ExtendedTinyBoyEmulator[nthreads];
		for(int i=0;i!=tinyBoys.length;++i) {
			ExtendedTinyBoyEmulator ith = createTinyBoy(gui);
			this.tinyBoys[i] = ith;
			if(gui) {
				JPeripheral view = ith.getView();
				int x = view.getX();
				int y = view.getY();
				view.setLocation(x + (50*i), y + (50*i));
			}
		}
		this.nthreads = nthreads;
		this.batchSize = batchSize;
	}

	/**
	 * Destroy all tinyboy instances created.
	 */
	public void destroy() {
		for(int i=0;i!=tinyBoys.length;++i) {
			tinyBoys[i].destroy();
		}
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
	 * @throws ExecutionException
	 * @throws InterruptedException
	 */
	public CoverageAnalysis run(double target) throws InterruptedException, ExecutionException {
		long time = System.currentTimeMillis();
		// Construct temporary memory areas
		@NonNull Iterator<Boolean>[][] arrays = new @NonNull Iterator[nthreads][batchSize];
		Future<Result[]>[] threads = new Future[nthreads];
		CoverageAnalysis analysis = new CoverageAnalysis(firmware);
		System.err.println("Initialised " + nthreads + " worker threads.");
		int iteration = 0;
		while (analysis.getBranchCoverage() < target && generator.hasMore()) {
			// Create next batch
			for (int i = 0; i != nthreads; ++i) {
				copyToArray(arrays[i], generator);
			}
			// Submit next batch for process
			for (int i = 0; i != nthreads; ++i) {
				final ExtendedTinyBoyEmulator tinyBoy = tinyBoys[i];
				final Iterator<Boolean>[] batch = arrays[i];
				threads[i] = executor.submit(() -> fuzzTest(tinyBoy,batch));
			}
			// Join all back together
			for (int i = 0; i != nthreads; ++i) {
				Result[] results = threads[i].get();
				final @NonNull Iterator<Boolean>[] batch = arrays[i];
				for(int j=0;j!=results.length;++j) {
					Result r = results[j];
					if(r != null) {
						// Result can be null for incomplete batches.
						BitSet covered = r.getCodeExecuted();
						// Insure only instructions returned.
						covered.and(analysis.getReachableInstructions());
						// Register the output with the generator so that it can refine its strategy
						// based on this.
						generator.record((T) batch[j], r.getCodeExecuted(), r.getState());
						// Record the output with the coverage analysis so that we can subsequently
						// compute coverage data.
						analysis.record(covered);
						// Update iteration count
						iteration = iteration + 1;
					}
				}
			}
			long t = System.currentTimeMillis() - time;
			double rate = Math.round((((double)iteration) / t) * 10000) / 10.0D;
			System.err.println("Processed " + iteration + " inputs @ " + rate + " inputs/s with coverage "
					+ Math.round(analysis.getBranchCoverage()) + "%");
		}
		return analysis;
	}

	/**
	 * Fuzz test a given batch of inputs.
	 *
	 * @param inputs
	 * @param cycles
	 * @return
	 */
	private Result[] fuzzTest(ExtendedTinyBoyEmulator tinyBoy, Iterator<Boolean>[] inputs) {
		Result[] r = new Result[inputs.length];
		for(int i=0;i!=r.length;++i) {
			Iterator<Boolean> input = inputs[i];
			if(input != null) {
				r[i] = fuzzTest(tinyBoy,input);
			}
		}
		return r;
	}
	/**
	 * Actually fuzz test the TinyBoy with a given sequence of input values.
	 *
	 * @param input
	 * @return
	 * @throws HaltedException
	 */
	private Result fuzzTest(ExtendedTinyBoyEmulator tinyBoy, Iterator<Boolean> input) {
		// Reset the tiny boy
		tinyBoy.reset();
		tinyBoy.upload(firmware);
		tinyBoy.bind(input);
		// Responsible for determining code locations which were read. This will include
		// all those which represent data.
		ReadWriteInstrument instrument = new ReadWriteInstrument();
		// Register instrumentation
		tinyBoy.getAVR().getCode().register(instrument);
		// Keep going until input is exhausted
		try {
			while(input.hasNext()) {
				tinyBoy.clock();
			}
		} catch (HaltedException e) {
		}
		// Remove instrumentation
		tinyBoy.getAVR().getCode().unregister(instrument);
		//
		byte @NonNull [] data = toByteArray(tinyBoy.getAVR().getData());
		// Extract the coverage data
		BitSet reads = instrument.getReads();
		assert reads != null;
		//
		return new Result(reads,data);
	}

	/**
	 * Create a TinyBoy emulator which has an optional graphical display.
	 *
	 * @return
	 */
	private ExtendedTinyBoyEmulator createTinyBoy(boolean gui) {
		SymbolicPullWire[] wires = new SymbolicPullWire[4];
		wires[ControlPad.Button.UP.ordinal()] = new SymbolicPullWire("PB1", "MISO", "DO", "AIN1", "OC0B", "OC1A", "PCINT1");
		wires[ControlPad.Button.DOWN.ordinal()] = new SymbolicPullWire("PB3", "PCINT3", "XTAL1", "CLK1", "!OC1B", "ADC3");
		wires[ControlPad.Button.LEFT.ordinal()] = new SymbolicPullWire("PB4", "PCINT4", "XTAL2", "CLK0", "OC1B", "ADC2");
		wires[ControlPad.Button.RIGHT.ordinal()] = new SymbolicPullWire("PB5", "PCINT5", "!RESET", "ADC0", "dW");
		return new ExtendedTinyBoyEmulator(wires,gui);
	}

	/**
	 * Pull inputs from the generator into one or more batches for execution.
	 * @param <T>
	 * @param array
	 * @param b
	 * @return
	 */
	private static <T extends Iterator<Boolean>> int copyToArray(Iterator<Boolean>[] array, InputGenerator<T> b) {
		int i = 0;
		// Read items into array
		while (b.hasMore() && i < array.length) {
			array[i++] = b.generate();
		}
		// Reset any trailing items
		Arrays.fill(array,i,array.length,null);
		// Done
		return i;
	}

	private static class ExtendedTinyBoyEmulator extends TinyBoyEmulator {
		private final SymbolicPullWire[] wires;
		private final JPeripheral view;

		public ExtendedTinyBoyEmulator(SymbolicPullWire[] wires, boolean gui) {
			super(labels -> getWire(wires,labels));
			this.wires = wires;
			if(gui) {
				this.view = new TinyBoyPeripheral(this);
			} else {
				this.view = null;
			}
		}

		public void bind(Iterator<Boolean> input) {
			for(int i=0;i!=wires.length;++i) {
				wires[i].bind(input);
			}
		}

		public JPeripheral getView() {
			return view;
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
		public void clock() throws HaltedException {
			final AVR mcu = this.getAVR();
			if(view != null) {
				// Clock peripheral first
				view.clock();
			}
			// Clock AVR second
			mcu.clock();
		}

		@Override
		public void destroy() {
			if(view != null) {
				view.setVisible(false);
				view.dispose();
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
		private Iterator<Boolean> input;

		public SymbolicPullWire(String... labels) {
			this.labels = labels;
			// Default input
			this.input = new Iterator<Boolean>() {

				@Override
				public boolean hasNext() {
					return false;
				}

				@Override
				public Boolean next() {
					return false;
				}

			};
		}

		public void bind(Iterator<Boolean> input) {
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
			return input.next();
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
	private byte @NonNull [] toByteArray(AVR.Memory memory) {
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
	public interface InputGenerator<T extends Iterator<Boolean>> {
		/**
		 * Get the next generated input.
		 *
		 * @return
		 */
		public @Nullable T generate();

		/**
		 * Record the result of a given test. That is, for a generated input, record the
		 * actual set of covered instructions along with the final state.
		 *
		 * @param input
		 * @param output
		 */
		public void record(@NonNull T input, @NonNull BitSet output, byte @NonNull [] state);

		/**
		 * Indicates whether or not the generator is finished.
		 *
		 * @return
		 */
		public boolean hasMore();
	}

	/**
	 * Represents the result from a fuzzing run.
	 *
	 * @author djp
	 *
	 */
	public static class Result {
		/**
		 * Code locations executed during a given run
		 */
		private final @NonNull BitSet code;
		/**
		 * State of machine memory at end of run.
		 */
		private final byte @NonNull [] state;

		public Result(@NonNull BitSet coverage, byte @NonNull [] state) {
			this.code = coverage;
			this.state = state;
		}

		/**
		 * Get the set of code locations executed during this run.
		 *
		 * @return
		 */
		public @NonNull BitSet getCodeExecuted() {
			return code;
		}

		/**
		 * Get the state of memory at the end of the this run.
		 *
		 * @return
		 */
		public byte @NonNull [] getState() {
			return state;
		}
	}
}
