import math
#data_len = 1500
data_len = 300
div_deg = 360/data_len
#counter_num = 1500
#counter_num = 1500
counter_num = 1500
sin_list = []
for i in range(data_len):
    sin_list.append(round(math.sin(math.radians(div_deg*i))*(counter_num/2)+(counter_num/2)))

print(sin_list)
print(0,(data_len*1/3),(data_len*2/3))
