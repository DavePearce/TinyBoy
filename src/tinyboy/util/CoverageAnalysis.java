package tinyboy.util;

import java.util.BitSet;

import javaavr.io.HexFile;
import javaavr.util.ElasticByteMemory;

/**
 * Provides a rudimentary tool for analyzing the (instruction and branch)
 * coverage obtained for a given firmware.
 *
 * @author David J. Pearce
 *
 */
public class CoverageAnalysis {
	/**
	 * Elastic memory provides a simple mechanism by which we can determine how much
	 * of the available SRAM was actually occupied by the firmware. This helps us to
	 * bound the maximum coverage we could obtain. Unfortunately, this is not itself
	 * enough to determine the true coverage value as we must also identify data and
	 * other unreachable instructions within the firmware.
	 */
	private ElasticByteMemory flash;

	/**
	 * Records the set of instructions which were covered by the given set of input
	 * values.
	 */
	private BitSet coverage;

	/**
	 *
	 * @param firmware
	 */
	public CoverageAnalysis(HexFile firmware) {
		flash = new ElasticByteMemory();
		coverage = new BitSet();
		firmware.uploadTo(flash);
	}

	/**
	 * Record a new set of coverage data.
	 *
	 * @param coverage
	 */
	public void record(BitSet coverage) {
		this.coverage.or(coverage);
	}

	/**
	 * Get the calculated instruction coverage from this analysis as a percentage.
	 *
	 * @return
	 */
	public double getInstructionCoverage() {
		return (100 * coverage.cardinality()) / flash.size();
	}
}
