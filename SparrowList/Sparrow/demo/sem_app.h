/*
 * MIT License
 *
 * Copyright (c) 2024 skaiui2

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *  https://github.com/skaiui2/SKRTOS_sparrow
 */

#ifndef SEM_APP_H
#define SEM_APP_H


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




#endif
