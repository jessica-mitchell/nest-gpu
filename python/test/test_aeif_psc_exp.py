import sys
import neuralgpu as ngpu

neuron = ngpu.Create('aeif_psc_exp', 1, 2)
ngpu.SetStatus(neuron, {"V_peak": 0.0, "a": 4.0, "b":80.5, "E_L":-70.6, \
                        "g_L":300.0, "tau_syn":[40.0, 20.0]})
spike = ngpu.Create("spike_generator")
spike_times = [10.0, 400.0]
n_spikes = 2

# set spike times and height
ngpu.SetStatus(spike, {"spike_times": spike_times})
delay = [1.0, 100.0]
#weight = [0.1, 0.2, 0.15]
weight = [1.0, -2.0]

conn_spec={"rule": "all_to_all"}


syn_spec_ex={'receptor':0, 'weight': weight[0], 'delay': delay[0]}
syn_spec_in={'receptor':1, 'weight': weight[1], 'delay': delay[1]}
ngpu.Connect(spike, neuron, conn_spec, syn_spec_ex)
ngpu.Connect(spike, neuron, conn_spec, syn_spec_in)

record = ngpu.CreateRecord("", ["V_m"], [neuron[0]], [0])
#voltmeter = nest.Create('voltmeter')
#nest.Connect(voltmeter, neuron)

ngpu.Simulate(800.0)

data_list = ngpu.GetRecordData(record)
t=[row[0] for row in data_list]
V_m=[row[1] for row in data_list]
#dmm = nest.GetStatus(voltmeter)[0]
#V_m = dmm["events"]["V_m"]
#t = dmm["events"]["times"]

import matplotlib.pyplot as plt

plt.figure(1)
plt.plot(t, V_m)

plt.draw()
plt.pause(1)
raw_input("<Hit Enter To Close>")
plt.close()
