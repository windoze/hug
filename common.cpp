#include "common.h"

std::mt19937& rng()
{
	static thread_local std::mt19937 gen(std::random_device{}());
	return gen;
}
