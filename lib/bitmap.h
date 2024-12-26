/**
 * @file bitmap.h
 * @brief Библиотека для работы с битовыми масками (Bitmaps).
 *
 * Эта библиотека предоставляет интерфейс для управления и операций с битовыми масками,
 * включая операции AND, OR, NOT, поиск установленных и сброшенных битов, а также их установку и сброс.
 *
 * Copyright 2019 Broadcom.
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "applog.h"

#define BMP_INVALID_ID -1

// Bitmap will be offset of 32bits and accessed using uint32_t
#define BMP_MASK_BITS 32
#define BMP_MASK_LEN 5
#define BMP_MASK 0x1f

#define BMP_FIRST16_MASK 0xffff0000
#define BMP_SECOND16_MASK 0x0000ffff

#define BMP_IS_BIT_POS_VALID(_bmp, _bit) ((_bit >= 0) && (_bit <= _bmp->nbits))

#define BMP_GET_ARR_ID(_p) (_p / BMP_MASK_BITS)
#define BMP_GET_ARR_POS(_p) ((_p) & BMP_MASK)

#define BMP_SET(_bmp, _k) (_bmp->arr[BMP_GET_ARR_ID(_k)] |= (1U << (BMP_GET_ARR_POS(_k))))
#define BMP_RESET(_bmp, _k) (_bmp->arr[BMP_GET_ARR_ID(_k)] &= ~(1U << (BMP_GET_ARR_POS(_k))))
#define BMP_ISSET(_bmp, _k) (_bmp->arr[BMP_GET_ARR_ID(_k)] & (1U << (BMP_GET_ARR_POS(_k))))

/**
 * @brief Структура для представления битовой маски.
 */
typedef struct BITMAP_S
{
    uint16_t nbits;    /**< Общее количество бит в битовой маске (0-65535). */
    uint16_t size;     /**< Размер массива, требуемого для хранения всех битов. 1-2047 */
    unsigned int *arr; /**< Указатель на массив, представляющий битовую маску. */
} BITMAP_T;

/**
 * @brief Тип идентификатора бита.
 */
typedef int32_t BMP_ID;

//
// For static allocation ,
// ex:
// BITMAP_T vlan_bmp;
// BMP_INIT_STATIC(vlan_bmp, 4096)
//
#define BMP_GET_ARR_SIZE_FROM_BITS(nbits) ((nbits + (BMP_MASK_BITS - 1)) / BMP_MASK_BITS)
#define BMP_INIT_STATIC(bmp, nbits)                              \
    do                                                           \
    {                                                            \
        unsigned int bmp_arr[BMP_GET_ARR_SIZE_FROM_BITS(nbits)]; \
        bmp.nbits = nbits;                                       \
        bmp.size = BMP_GET_ARR_SIZE_FROM_BITS(nbits);            \
        bmp.arr = &bmp_arr[0];                                   \
    } while (0);

/**
 * @brief Сравнивает две битовые маски.
 * @param bmp1 Первая битовая маска.
 * @param bmp2 Вторая битовая маска.
 * @return true, если маски равны, иначе false.
 */
bool bmp_is_mask_equal(BITMAP_T *bmp1, BITMAP_T *bmp2);

/**
 * @brief Копирует содержимое одной битовой маски в другую.
 * @param dst Целевая битовая маска.
 * @param src Исходная битовая маска.
 */
void bmp_copy_mask(BITMAP_T *dst, BITMAP_T *src);

/**
 * @brief Применяет операцию NOT к битовой маске.
 * @param dst Целевая битовая маска.
 * @param src Исходная битовая маска.
 */
void bmp_not_mask(BITMAP_T *dst, BITMAP_T *src);

/**
 * @brief Применяет операцию AND к двум битовым маскам.
 * @param tgt Целевая битовая маска.
 * @param bmp1 Первая битовая маска.
 * @param bmp2 Вторая битовая маска.
 */
void bmp_and_masks(BITMAP_T *tgt, BITMAP_T *bmp1, BITMAP_T *bmp2);

/**
 * @brief Применяет операцию AND с NOT ко второй битовой маске.
 * @param tgt Целевая битовая маска.
 * @param bmp1 Первая битовая маска.
 * @param bmp2 Вторая битовая маска.
 */
void bmp_and_not_masks(BITMAP_T *tgt, BITMAP_T *bmp1, BITMAP_T *bmp2);

/**
 * @brief Применяет операцию OR к двум битовым маскам.
 * @param tgt Целевая битовая маска.
 * @param bmp1 Первая битовая маска.
 * @param bmp2 Вторая битовая маска.
 */
void bmp_or_masks(BITMAP_T *tgt, BITMAP_T *bmp1, BITMAP_T *bmp2);

/**
 * @brief Применяет операцию XOR к двум битовым маскам.
 * @param tgt Целевая битовая маска.
 * @param bmp1 Первая битовая маска.
 * @param bmp2 Вторая битовая маска.
 */
void bmp_xor_masks(BITMAP_T *tgt, BITMAP_T *bmp1, BITMAP_T *bmp2);

/**
 * @brief Проверяет, установлен ли хотя бы один бит в битовой маске.
 * @param bmp Битовая маска.
 * @return true, если хотя бы один бит установлен, иначе false.
 */
bool bmp_isset_any(BITMAP_T *bmp);

/**
 * @brief Проверяет, установлен ли конкретный бит в битовой маске.
 * @param bmp Битовая маска.
 * @param bit Позиция бита.
 * @return true, если бит установлен, иначе false.
 */
bool bmp_isset(BITMAP_T *bmp, uint16_t bit);

/**
 * @brief Устанавливает конкретный бит в битовой маске.
 * @param bmp Битовая маска.
 * @param bit Позиция бита.
 */
void bmp_set(BITMAP_T *bmp, uint16_t bit);

/**
 * @brief Устанавливает все биты в битовой маске.
 * @param bmp Битовая маска.
 */
void bmp_set_all(BITMAP_T *bmp);

/**
 * @brief Сбрасывает конкретный бит в битовой маске.
 * @param bmp Битовая маска.
 * @param bit Позиция бита.
 */
void bmp_reset(BITMAP_T *bmp, uint16_t bit);

/**
 * @brief Сбрасывает все биты в битовой маске.
 * @param bmp Битовая маска.
 */
void bmp_reset_all(BITMAP_T *bmp);

/**
 * @brief Выводит все значения битовой маски.
 * @param bmp Битовая маска.
 */
void bmp_print_all(BITMAP_T *bmp);

/**
 * @brief Инициализирует битовую маску.
 * @param bmp Указатель на битовую маску.
 * @return 0 при успешной инициализации, иначе -1.
 */
int8_t bmp_init(BITMAP_T *bmp);

/**
 * @brief Выделяет память и инициализирует битовую маску.
 * @param bmp Указатель на указатель битовой маски.
 * @param nbits Количество бит.
 * @return 0 при успешной аллокации, иначе -1.
 */
int8_t bmp_alloc(BITMAP_T **bmp, uint16_t nbits);

/**
 * @brief Освобождает память, выделенную под битовую маску.
 * @param bmp Указатель на битовую маску.
 */
void bmp_free(BITMAP_T *bmp);

BMP_ID bmp_find_first_unset_bit_after_offset(BITMAP_T *bmp, uint16_t offset);
BMP_ID bmp_set_first_unset_bit_after_offset(BITMAP_T *bmp, uint16_t offset);
BMP_ID bmp_find_first_unset_bit(BITMAP_T *bmp);
BMP_ID bmp_set_first_unset_bit(BITMAP_T *bmp);
BMP_ID bmp_get_next_set_bit(BITMAP_T *bmp, BMP_ID id);
BMP_ID bmp_get_first_set_bit(BITMAP_T *bmp);

#endif //__BITMAP_H__
