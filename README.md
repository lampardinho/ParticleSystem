# ParticleSystem

Update of particles' positions happens in the separate thread in the ```WorkerThread``` function. For speeding up the particles proccesing the buffer is splitted in parts which are updated asynchroniously.

Tripple buffering is used to eliminate dependence between update and render. In case render takes too long and both backbuffers are already filled, the next position calculation results are stored in the older backbuffer.

Thread synchronization is needed during buffers swapping and adding/deleting of particle effects.

Ring buffer is used to store particles. It cannot overflow, stores its elements sequencially in memmory and provides fast removing and adding operations, since by design the elements are sorted in order of their creation and all have constant lifetime.

Following articles were used during the implementation:

http://cowboyprogramming.com/2007/01/05/multithreading-particle-sytems/

https://software.intel.com/en-us/blogs/2011/02/18/building-a-highly-scalable-3d-particle-system
