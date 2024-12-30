//
// Created by el on 2024/11/21.
//

#ifndef STM32F1_SEM_APP_H
#define STM32F1_SEM_APP_H


typedef struct Oo_buffer *Oo_buffer_handle;
Oo_buffer_handle Oo_buffer_creat(void);
void Oo_insert(Oo_buffer_handle Oo_buffer1, int object);
int Oo_remove(Oo_buffer_handle Oo_buffer1);


typedef struct Mo_buffer *Mo_buffer_handle;
Mo_buffer_handle Mo_buffer_creat(void);
void Mo_insert(Mo_buffer_handle Mo_buffer1, int object);
int Mo_remove(Mo_buffer_handle Mo_buffer1);


typedef struct Mm_buffer *Mm_buffer_handle;
Mm_buffer_handle Mm_buffer_creat(void);
void Mm_insert(Mm_buffer_handle Mm_buffer1, int object);
int Mm_remove(Mm_buffer_handle Mm_buffer1);


typedef struct MrMw_control *MrMw_control_handle;
MrMw_control_handle MrOw_creat(void);
void read_acquire(MrMw_control_handle MrMw_control1);
void read_release(MrMw_control_handle MrMw_control1);
void write_acquire(MrMw_control_handle MrMw_control1);
void write_release(MrMw_control_handle MrMw_control1);




#endif //STM32F1_SEM_APP_H
