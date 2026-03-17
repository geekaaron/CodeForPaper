import json
import sys
import matplotlib.pyplot as plt
import sg, pt, seg

plt.rc('font', family='Times New Roman')

if len(sys.argv) != 2:
    print("Usage: python compare.py <json>")
    sys.exit(0)

with open(sys.argv[1], "r", encoding="utf-8") as f:
    data = json.load(f)

_, _, seg_p, seg_r = seg.run_seg(data)
_, _, sg_p, sg_r = sg.run_sg(data)
_, _, pt_p, pt_r = pt.run_sg_pt(data)

print("=================== SEG ===================")
print("Provider average payoff:", seg_p[-1])
print("Requester average payoff:", seg_r[-1])
print()

print("=================== SG ====================")
print("Provider average payoff:", sg_p[-1])
print("Requester average payoff:", sg_r[-1])
print()

print("=================== PT ====================")
print("Provider average payoff:", pt_p[-1])
print("Requester average payoff:", pt_r[-1])
print()

fig, ax = plt.subplots(1 , 3, figsize=(15, 4))

ax[0].set_xlabel("Iteration")
ax[0].set_ylabel("Payoff")
ax[0].plot(seg_p, label="SEG")
ax[0].plot(sg_p, label="SG")
ax[0].plot(pt_p, label="SGPT")
ax[0].set_title("(a) Provider's Total Payoff under Different models")
ax[0].legend()

ax[1].set_xlabel("Iteration")
ax[1].set_ylabel("Payoff")
ax[1].plot(seg_r, label="SEG")
ax[1].plot(sg_r, label="SG")
ax[1].plot(pt_r, label="SGPT")
ax[1].set_title("(b) Requester's Total Payoff under Different models")
ax[1].legend()

ax[2].set_xlabel("Iteration")
ax[2].set_ylabel("Payoff")
ax[2].plot(seg_p + seg_r, label="SEG")
ax[2].plot(sg_p + sg_r, label="SG")
ax[2].plot(pt_p+ pt_r, label="SGPT")
ax[2].set_title("(c) Social Welfare under Different models")
ax[2].legend()

plt.tight_layout()
plt.show()
