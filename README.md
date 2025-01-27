# NEST GPU

**NEST GPU was developed under the name [NeuronGPU](https://github.com/golosio/NeuronGPU) before it has been integrated in the NEST Initiative, see [Golosio et al. (2021)](https://www.frontiersin.org/articles/10.3389/fncom.2021.627620/full). Currently this repository is being adapted to the NEST development workflow.**

A GPU-MPI library for simulation of large-scale networks of spiking neurons.
Can be used in Python, in C++ and in C.

With this library it is possible to run relatively fast simulations of large-scale networks of spiking neurons. For instance, on a single Nvidia GeForce RTX 2080 Ti GPU board it is possible to simulate the activity of 1 million multisynapse AdEx neurons with 1000 synapse per neurons, for a total of 1 billion synapse, using the fifth-order Runge-Kutta method with adaptive stepsize as differential equations solver, in little more than 70 seconds per second of neural activity.
The MPI communication is also very efficient.
The python interface is very similar to that of the NEST simulator: the most used commands are practically identical, dictionaries are used to define neurons, connections and synapsis properties in the same way.

To start using it,, have a look at the examples in the python/examples and c++/examples folders.

* **[Download and installation instructions](https://github.com/golosio/NESTGPU/wiki/Installation-instructions)**
* **[User guide](https://github.com/golosio/NESTGPU/wiki)**
