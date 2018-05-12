package tinyboy;

import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

import javaavr.core.AVR;
import javaavr.core.AvrDecoder;
import javaavr.core.AvrExecutor;
import javaavr.core.Wire;
import javaavr.io.HexFile;
import javaavr.peripherals.DotMatrixDisplay;
import javaavr.util.ByteMemory;
import javaavr.util.IoMemory;
import javaavr.util.MultiplexedMemory;
import javaavr.util.WireArrayPort;
import tinyboy.core.TinyBoyEmulator;

/**
 * A simple tool for generate test coverage information for the AVR simulator.
 *
 * @author David J. Pearce
 *
 */
public class Main {

	public static void main(String[] args) throws IOException {
		// Read hexfile and upload to emulator
		HexFile.Reader hfr = new HexFile.Reader(new FileReader("tetris.hex"));
		TinyBoyEmulator tbem = new TinyBoyEmulator();
		tbem.upload(hfr.readAll());
		// Print out display
		System.out.println(tbem.getOutput());
		// Clock 100000 cycles
		for (int j = 0; j != 10; ++j) {
			for (int i = 0; i != 1000000; ++i) {
				tbem.clock();
			}
			// System.out.println(tbem.getOutput());
		}
		System.out.println("--");
		System.out.println(tbem.getOutput());
		System.out.println(tbem.getCoverage().cardinality() + " / 8192");
	}

}
