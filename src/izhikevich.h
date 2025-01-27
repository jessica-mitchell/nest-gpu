/*
 *  This file is part of NESTGPU.
 *
 *  Copyright (C) 2021 The NEST Initiative
 *
 *  NESTGPU is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NESTGPU is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NESTGPU.  If not, see <http://www.gnu.org/licenses/>.
 *
 */





#ifndef IZHIKEVICHH
#define IZHIKEVICHH

#include <iostream>
#include <string>
#include "cuda_error.h"
#include "node_group.h"
#include "base_neuron.h"
#include "neuron_models.h"


namespace izhikevich_ns
{
enum ScalVarIndexes {
  i_I_syn = 0,        // input current
  i_V_m,              // membrane potential
  i_u,
  i_refractory_step,  // refractory step counter
  N_SCAL_VAR
};

enum ScalParamIndexes {
  i_I_e = 0,         // External current in pA
  i_den_delay,
  N_SCAL_PARAM
};

enum GroupParamIndexes {
  i_V_th = 0,
  i_a,
  i_b,
  i_c,
  i_d,
  i_t_ref,           // Refractory period in ms
  N_GROUP_PARAM
};


 
const std::string izhikevich_scal_var_name[N_SCAL_VAR] = {
  "I_syn",
  "V_m",
  "u",
  "refractory_step"
};

const std::string izhikevich_scal_param_name[N_SCAL_PARAM] = {
  "I_e",
  "den_delay"
};

const std::string izhikevich_group_param_name[N_GROUP_PARAM] = {
  "V_th",
  "a",
  "b",
  "c",
  "d",
  "t_ref"
};
 
} // namespace
 



class izhikevich : public BaseNeuron
{
  float time_resolution_;

 public:
  ~izhikevich();
  
  int Init(int i_node_0, int n_neuron, int n_port, int i_group,
	   unsigned long long *seed);
  int Calibrate(double /*time_min*/, float time_res) {
    time_resolution_ = time_res;
    return 0;
  }
  
  int Update(long long it, double t1);

  int Free();

};


#endif
