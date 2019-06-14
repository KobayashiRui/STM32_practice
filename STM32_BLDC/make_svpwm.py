import math
#data_len = 1500
data_len = 1500
div_deg = 360/data_len
#counter_num = 1500
#counter_num = 1500
counter_num = 1500
data_list = []
for i in range(data_len):
    sin_data_A = counter_num*math.sin(math.radians(div_deg*i))
    sin_data_B = counter_num*math.sin(math.radians(div_deg*i-120))
    sin_data_C = counter_num*math.sin(math.radians(div_deg*i+120))
    sin_list=[sin_data_A,sin_data_B,sin_data_C]
    voff = (min(sin_list)+max(sin_list))/2
    svpwm_A = sin_data_A-voff
    data_list.append(round((svpwm_A/2) + counter_num/2))

for data in data_list[:-1]:
    print(data,end=",")
print(data_list[-1])
print(len(data_list))
print(0,(data_len*1/3),(data_len*2/3))
