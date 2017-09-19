#include "bus.h"

template<unsigned int B, unsigned int S>
int Bus<B,S>::AddBusComponent(BusComponentBase *newComponent) {

    for (unsigned int dd = 0; dd < m_components.size(); d++) {
        if (m_components.at(dd) == newComponent )
          return -1;
    }
    m_components.push_back(newComponent);
    OnComponentsUpdated();
    return 0;
}

template<unsigned int B, unsigned int S>
int Bus<B,S>::RemoveBusComponent(BusComponentBae *component) {

  for (unsigned int dd = 0; dd < m_components.size(); dd++) {
      if (m_components.at(dd) == component) {
          m_components.erase(m_components.begin()+dd);
          OnComponentsUpdated();
          return 0;
      }
  }

  return -1;
}