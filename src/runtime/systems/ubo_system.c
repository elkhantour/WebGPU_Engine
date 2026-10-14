#include "ubo_system.h"
#include "backend/ubo.h"


void ubo_system_draw_callback(void *data) {

  UBOManager *manager = (UBOManager *)data;

  for (UBOType type = 0; type < UBO_TYPE_COUNT; type++) {
    UBOBufferUpdateQueue *queue = &manager->buffers[type].update_queue;

    while (queue->count > 0) {
      ubo_id_t id = queue->entries[0];

      ubo_upload_entry(manager, type,
                       &(UBOSlot){
                           .id = id,
                           .uniform = &manager->buffers[type].entries[id],
                       });
      ubo_update_queue_shift(manager, type);
    }
  }
}
