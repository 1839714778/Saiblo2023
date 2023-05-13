from keras.models import Model
from keras import layers
from keras import Input

input=Input(shape=(1995,),dtype='float32',name='input')
layer1=layers.Dense(64,activation='relu')(input)
output1=layers.Dense(1,activation='tanh')(layer1)
output2=layers.Dense(133,activation='softmax')(layer1)
answer=layers.concatenate([output1,output2],axis=-1)
network=Model(input,answer)
network.save('model_a0.h5',include_optimizer=False)
