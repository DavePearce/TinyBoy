package tinyboy.util;

import java.util.BitSet;

import javaavr.core.AVR.Memory;

/**
 * This memory records every location which has been read. This allows us to
 * determine which instructions have been accessed during the execution of a
 * given test case.
 *
 * @author David J. Pearce
 *
 */
public class ProfiledMemory implements Memory {
	private final BitSet allReads;
	private final Memory mem;

	public ProfiledMemory(Memory mem) {
		this.mem = mem;
		this.allReads = new BitSet();
	}

	public boolean wasRead(int address) {
		return allReads.get(address);
	}

	public BitSet getReads() {
		return allReads;
	}

	@Override
	public byte read(int address) {
		allReads.set(address);
		return mem.read(address);
	}

	@Override
	public byte peek(int address) {
		return mem.peek(address);
	}

	@Override
	public void write(int address, byte data) {
		mem.write(address, data);
	}

	@Override
	public void write(int address, byte[] data) {
		mem.write(address, data);
	}

	@Override
	public void poke(int address, byte data) {
		mem.poke(address, data);
	}

	@Override
	public int size() {
		return mem.size();
	}
}
