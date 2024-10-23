#include <cstdint>
/*
	DS_CRC32.h is part of DualSenseWindows
	https://github.com/Ohjurot/DualSense-Windows

	Contributors of this file:
	11.2020 Ludwig F�chsl

	Licensed under the MIT License (To be found in repository root directory)
*/
#pragma once

#include "DSW_Api.h"
#include "Device.h"
#include "DS5State.h"


namespace __DS5W {
	/// <summary>
	/// Pre gennerated CRC hashing for DS5
	/// </summary>
	class CRC32 {
	private:
		/// <summary>
		/// Fast lookup precalculated byte crc hashes
		/// </summary>
		const static uint32_t hashTable[256];

		/// <summary>
		/// Start seed for crc hash
		/// </summary>
		const static uint32_t crcSeed;


	public:
		/// <summary>
		/// Compute the CRC32 Hash
		/// </summary>
		/// <param name="buffer">Input buffer</param>
		/// <param name="len">Length of buffer</param>
		/// <returns>Computed crc value</returns>
		static uint32_t compute(unsigned char* buffer, size_t len);
	};
}
