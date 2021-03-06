#pragma once

#include "Core/Array.hpp"
#include "Core/Queue.hpp"
#include "Entity/Entity.hpp"
#include "Memory/Allocator.hpp"

class EntityManager
{
private:
	static const unsigned int MinimumFreeIndices = 1024;

	Allocator* allocator;
	Array<unsigned char> generation;
	Queue<unsigned> freeIndices;

public:
	EntityManager(Allocator* allocator) :
		allocator(allocator),
		generation(allocator),
		freeIndices(allocator)
	{
		// Reserve index 0 as invalid value
		generation.PushBack(0);
	}

	~EntityManager()
	{
	}

	Entity Create()
	{
		unsigned int idx;
		if (freeIndices.GetCount() > MinimumFreeIndices)
		{
			idx = freeIndices.Peek();
			freeIndices.Pop();
		}
		else
		{
			generation.PushBack(0);
			idx = generation.GetCount() - 1;
		}
		return Entity::Make(idx, generation[idx]);
	}

	bool IsAlive(Entity e) const { return generation[e.Index()] == e.Generation(); }

	void Destroy(Entity e)
	{
		const unsigned idx = e.Index();
		++generation[idx];
		freeIndices.Push(idx);
	}
};
