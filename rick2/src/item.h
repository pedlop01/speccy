#ifndef ITEM_H
#define ITEM_H

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "rick_params.h"
#include "object.h"

class Item : public Object {
  private:
    int steps_dying;

  private:
    void UpdateFSMState();

  public:
    Item();    
    ~Item();

    // Re-write computeation of next position
    void ComputeNextPosition(World* map);
    // Re-write step from parent class
    void ObjectStep(World* map, Character* player);
};

#endif // PLATFORM_H