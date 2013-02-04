/* 
 * File:   sensor_data.h
 * Author: enrico
 *
 * Created on 4. Februar 2013, 07:46
 */

#ifndef SENSOR_DATA_H
#define	SENSOR_DATA_H

/**
 * @addtogroup default_app
 * 
 * @{
 */
/**
 * @defgroup sensor_data Sensor data struct definition
 * 
 * @{
 */

#ifndef bool_t
#define bool_t int
#endif
// pointer to processor function, first value is possible storage location
typedef bool_t(*processor_p)(void*, vec3i_t*);
//typedef bool_t(*processor_p)(sensor_data_t* sensor_data);
//typedef bool_t(*processor_p)(void*, int*);

typedef struct vec3i_s {
  int x;
  int y;
  int z;
} vec3i_t;

typedef struct acc_data_s {
  bool_t enabled;
  // raw axis value
  vec3i_t raw;
  // processed axis values
  vec3i_t out;
  // 3 channel processors
  processor_p processor[5];
  /// number of registered processors
  int processors;
  /// processor for x_axis
  void (*x_processor[5])(int, int);
  /// processor for y_axis
  void (*y_processor[5])(int, int);
  /// processor for z_axis
  void (*z_processor[5])(int, int);
  /// holds values to compare with
  int processor_comp[5];
} acc_data_t;

/**
 */
typedef struct gyro_data_s {
  bool_t enabled;
  vec3i_t raw;
  vec3i_t out;
} gyro_data_t;

/**
 */
typedef struct pressure_data_s {
  bool_t enabled;
  int raw;
  int out;
} pressure_data_t;

/**
 */
typedef struct temp_data_s {
  bool_t enabled;
  int raw;
  int out;
} temp_data_t;

/**
 */
typedef struct sensor_data_s {
  acc_data_t acc;
  gyro_data_t gyro;
  pressure_data_t pressure;
  temp_data_t temp;
  bool_t updated;
} sensor_data_t;

void doit(void) {
  int i;
  for (i = 0; i < acc_data_t.processors; i++) {
    acc_data_t.processor[i](1, 2);
  }
}

// standard processors (1 argument)

bool_t proc_eq(int a, int b) {
  return a == b;
}

bool_t proc_gt(int a, int b) {
  return a > b;
}

bool_t proc_lt(int a, int b) {
  return a < b;
}

bool_t proc_geq(int a, int b) {
  return a >= b;
}

bool_t proc_leq(int a, int b) {
  return a <= b;
}

bool_t proc_add(void* ret, int a, int b) {
  (int*) ret = a + b;
}

/** @} */
/** @} */

#endif	/* SENSOR_DATA_H */

