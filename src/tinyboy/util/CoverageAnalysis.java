package tinyboy.util;

import java.io.FileReader;
import java.io.IOException;
import java.util.BitSet;

import javr.core.AVR.Memory;
import javr.core.AvrDecoder;
import javr.core.AvrInstruction;
import javr.core.AvrInstruction.FlagRelativeAddress;
import javr.core.AvrInstruction.RegisterRegister;
import javr.core.AvrInstruction.RelativeAddress;
import javr.io.HexFile;
import javr.memory.ElasticByteMemory;

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
	 * The disassembly of the firmware image. This is necessary to determine an
	 * accurate coverage value.
	 */
	private final AvrInstruction[] disassembly;

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
		this.disassembly = disassemble(flash);
	}

	/**
	 * Record a new set of coverage data.
	 *
	 * @param coverage
	 */
	public void record(BitSet coverage) {
		this.coverage.or(coverage);
	}

	public boolean wasCovered(int pc) {
		return coverage.get(pc * 2);
	}

	public boolean isReachableInstruction(int address) {
		return address < disassembly.length && disassembly[address] != null;
	}

	public boolean isConditionalBranch(int address) {
		return isReachableInstruction(address) && isConditionalBranch(disassembly[address]);
	}

	public boolean isConditionalBranchCovered(int pc) {
		if (isConditionalBranch(pc)) {
			AvrInstruction instruction = disassembly[pc];
			pc = pc + instruction.getWidth();
			int target;
			switch (instruction.getOpcode()) {
			case BRBC:
			case BRBS: {
				FlagRelativeAddress branch = (FlagRelativeAddress) instruction;
				target = pc + branch.k;
				break;
			}
			case BRCC:
			case BRCS:
			case BREQ:
			case BRGE:
			case BRHC:
			case BRHS:
			case BRID:
			case BRIE:
			case BRLO:
			case BRLT:
			case BRMI:
			case BRNE:
			case BRPL:
			case BRSH:
			case BRTC:
			case BRTS:
			case BRVC:
			case BRVS: {
				RelativeAddress branch = (RelativeAddress) instruction;
				target = pc + branch.k;
				//
				break;
			}
			case CPSE:
			case SBIC:
			case SBIS:
			case SBRC:
			case SBRS: {
				AvrInstruction following = disassembly[pc];
				target = pc + following.getWidth();
				//
				break;
			}
			default:
				throw new IllegalArgumentException("invalid conditional branch encountered: " + instruction);
			}
			// PC at next instruction
			// TARGET at branch destination
			return wasCovered(pc) && wasCovered(target);
		} else {
			return false;
		}
	}

	public BitSet getReachableInstructions() {
		BitSet b = new BitSet();
		for (int i = 0; i != disassembly.length; ++i) {
			AvrInstruction insn = disassembly[i];
			if (insn != null) {
				for(int j=0;j!=insn.getWidth();++j) {
					b.set((i<<1)+j);
				}
			}
		}
		return b;
	}

	/**
	 * Get the calculated instruction coverage from this analysis as a percentage.
	 *
	 * @return
	 */
	public double getInstructionCoverage() {
		int count = 0;
		int covered = 0;
		// Count total number of reachable instructions
		for (int i = 0; i != disassembly.length; ++i) {
			if (disassembly[i] != null) {
				count = count + 1;
				covered += coverage.get(i*2) ? 1 : 0;
			}
		}
		return (100.0 * covered) / count;
	}


	/**
	 * Get the branch coverage from this analysis as a percentage.
	 *
	 * @return
	 */
	public double getBranchCoverage() {
		int count = 0;
		int covered = 0;
		// Count total number of reachable instructions
		for (int i = 0; i != disassembly.length; ++i) {
			if (isConditionalBranch(i)) {
				count = count + 1;
				covered += isConditionalBranchCovered(i) ? 1 : 0;
			}
		}
		return (100.0 * covered) / count;
	}

	private static AvrInstruction[] disassemble(Memory memory) {
		// NOTE: div 2 because instructions are either 16bit or 32bit.
		AvrDecoder decoder = new AvrDecoder();
		AvrInstruction[] instructions = new AvrInstruction[memory.size() / 2];
		disassemble(0,instructions,decoder,memory);
		return instructions;
	}

	private static void disassemble(int pc, AvrInstruction[] instructions, AvrDecoder decoder, Memory memory) {
		if(instructions[pc] != null) {
			// Indicates this instruction has already been visited. Therefore, we can ignore
			// it as we don't need to recompue the value.
			return;
		} else {
			instructions[pc] = decoder.decode(memory, pc);
			dispatch(pc,instructions,decoder,memory);
		}
	}

	private static void dispatch(int pc, AvrInstruction[] instructions, AvrDecoder decoder, Memory memory) {
		AvrInstruction instruction = instructions[pc];
		// Move to the next logical instruction as this is always the starting point.
		pc = pc + instruction.getWidth();
		//
		switch (instruction.getOpcode()) {
		case BRBC:
		case BRBS:{
			FlagRelativeAddress branch = (FlagRelativeAddress) instruction;
			// Explore the false branch
			disassemble(pc, instructions, decoder, memory);
			// Explore the true branch
			disassemble(pc + branch.k, instructions, decoder, memory);
			//
			break;
		}
		case BRCC:
		case BRCS:
		case BREQ:
		case BRGE:
		case BRHC:
		case BRHS:
		case BRID:
		case BRIE:
		case BRLO:
		case BRLT:
		case BRMI:
		case BRNE:
		case BRPL:
		case BRSH:
		case BRTC:
		case BRTS:
		case BRVC:
		case BRVS: {
			RelativeAddress branch = (RelativeAddress) instruction;
			// Explore the false branch
			disassemble(pc, instructions, decoder, memory);
			// Explore the true branch
			disassemble(pc + branch.k, instructions, decoder, memory);
			//
			break;
		}
		case CPSE:
		case SBIC:
		case SBIS:
		case SBRC:
		case SBRS:{
			// Explore the false branch
			disassemble(pc, instructions, decoder, memory);
			// Look at following instruction to know how to skip it.
			AvrInstruction following = instructions[pc];
			// Explore the true branch (i.e. after skipped instruction)
			disassemble(pc + following.getWidth(), instructions, decoder, memory);
			//
			break;
		}
		case CALL: {
			RelativeAddress branch = (RelativeAddress) instruction;
			// Explore call target
			disassemble(branch.k, instructions, decoder, memory);
			// Continue after call
			disassemble(pc, instructions, decoder, memory);
			//
			break;
		}
		case RCALL: {
			RelativeAddress branch = (RelativeAddress) instruction;
			// Explore call target
			disassemble(pc + branch.k, instructions, decoder, memory);
			// Continue after call
			disassemble(pc, instructions, decoder, memory);
			//
			break;
		}
		case JMP: {
			RelativeAddress branch = (RelativeAddress) instruction;
			// Explore the branch target
			disassemble(branch.k, instructions, decoder, memory);
			//
			break;
		}
		case RJMP: {
			RelativeAddress branch = (RelativeAddress) instruction;
			// Explore the branch target
			disassemble(pc + branch.k, instructions, decoder, memory);
			//
			break;
		}
		case EICALL:
		case EIJMP:
		case ICALL:
		case IJMP:
			// TODO: there is no support for indirect jumps. This is obviously a problem for
			// jump tables which are generated from switch statements.
			throw new IllegalArgumentException("indirect branch encountered");

		case RET:
		case RETI:
			// Return instructions just terminate because their return address is already explore separately.
			return;
		default:
			// Indicates a standard instruction where control is transferred to the
			// following instruction.
			disassemble(pc, instructions, decoder, memory);
		}
	}

	private static boolean isConditionalBranch(AvrInstruction instruction) {
		switch(instruction.getOpcode()) {
		case BRBC:
		case BRBS:
		case BRCC:
		case BRCS:
		case BREQ:
		case BRGE:
		case BRHC:
		case BRHS:
		case BRID:
		case BRIE:
		case BRLO:
		case BRLT:
		case BRMI:
		case BRNE:
		case BRPL:
		case BRSH:
		case BRTC:
		case BRTS:
		case BRVC:
		case BRVS:
		case CPSE:
		case SBIC:
		case SBIS:
		case SBRC:
		case SBRS:
			return true;
		default:
			return false;
		}
	}
}
