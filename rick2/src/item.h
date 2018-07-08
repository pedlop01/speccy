#ifndef ITEM_H
#define ITEM_H

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "rick_params.h"
#include "object.h"

class Item : public Object {
  private:

  private:
    void UpdateFSMState();

  public:
    Item();    
    ~Item();

    // Re-write step from parent class
    void ObjectStep(World* map, Character* player);
};

#endif // PLATFORM_H