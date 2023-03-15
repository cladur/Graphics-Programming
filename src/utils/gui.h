#ifndef GUI_H
#define GUI_H

#include <vector>

#include "../graphics/entity.h"
#include "../graphics/light.h"
#include "../graphics/model.h"

void renderSphere();
void ShowExampleAppDockSpace(bool* p_open);
void entityEntry(Entity* entity, Entity** entity_clicked, std::vector<Entity*>* entities_selected);

#endif
