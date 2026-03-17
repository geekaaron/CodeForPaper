import numpy as np
import matplotlib.pyplot as plt

plt.rcParams['font.family']=' Times New Roman, SimSun'

NUM_NODES = 30
ROUNDS = 100
PER_ROUND = 10
np.random.seed(42)

LAM_LIST = [0.8, 0.5, 0.2]
LINESTYLE = {0.8: "-", 0.5: "--", 0.2: ":"}

NODE_TYPES = (["可信"] * 10 + ["懒惰"] * 10 + ["恶意"] * 10)

def avg_rep(rep, node_type):
    idx = [i for i, t in enumerate(NODE_TYPES) if t == node_type]
    return rep[idx].mean(axis=0)

# =========================================================
# (a) 数据提供者声誉
# =========================================================

alpha1, alpha2, alpha3 = 0.4,0.3,0.3
beta1, beta2, beta3 = 0.4,0.3,0.3

BEHAVIOR = {
"可信":{"participation":0.9,"consensus":0.95,"ontime":0.95,"quality":(0.8,1.0)},
"懒惰":{"participation":0.1,"consensus":1.0,"ontime":0.9,"quality":(0.7,0.9)},
"恶意":{"participation":0.7,"consensus":0.4,"ontime":0.4,"quality":(0.1,0.4)}
}

DTC=np.zeros(NUM_NODES)
OTDC=np.zeros(NUM_NODES)
CC=np.zeros(NUM_NODES)
CSC=np.zeros(NUM_NODES)
ADQ_sum=np.zeros(NUM_NODES)

history={lam:np.zeros((NUM_NODES,ROUNDS)) for lam in LAM_LIST}

for r in range(ROUNDS):

    for _ in range(PER_ROUND):

        i=np.random.randint(NUM_NODES)
        b=BEHAVIOR[NODE_TYPES[i]]

        if np.random.rand()>b["participation"]:
            continue

        DTC[i]+=1
        CC[i]+=1

        if np.random.rand()<b["consensus"]:
            CSC[i]+=1

        if np.random.rand()<b["ontime"]:
            OTDC[i]+=1

        ADQ_sum[i]+=np.random.uniform(*b["quality"])

    CSR=np.divide(CSC,CC,out=np.zeros_like(CSC),where=CC>0)
    OTDR=np.divide(OTDC,DTC,out=np.zeros_like(OTDC),where=DTC>0)
    ADQ=np.divide(ADQ_sum,DTC,out=np.zeros_like(ADQ_sum),where=DTC>0)

    R_local=alpha1*CSR+alpha2*OTDR+alpha3*ADQ

    w_consensus=CC/(np.max(CC)+1e-9)
    w_transaction=DTC/(np.max(DTC)+1e-9)
    w_quality=ADQ/(np.max(ADQ)+1e-9)

    R_global=(beta1*w_consensus*CSR+
              beta2*w_transaction*OTDR+
              beta3*w_quality*ADQ)

    for lam in LAM_LIST:
        history[lam][:,r]=lam*R_local+(1-lam)*R_global

plt.figure(figsize=(6,4))

for node_type in ["可信","懒惰","恶意"]:
    for lam in LAM_LIST:

        plt.plot(
            avg_rep(history[lam],node_type),
            linestyle=LINESTYLE[lam],
            linewidth=2,
            label=f"{node_type} (λ={lam})"
        )

plt.xlabel("迭代次数",fontsize=18)
plt.ylabel("声誉",fontsize=18)
plt.ylim(0,1)
plt.tick_params(labelsize=14)

handles,labels = plt.gca().get_legend_handles_labels()

plt.tight_layout()
plt.show()

# =========================================================
# (b) 数据请求者声誉
# =========================================================

alpha1,alpha2=0.5,0.5
beta1,beta2=0.5,0.5

BEHAVIOR={
"可信":{"participation":0.9,"consensus":0.95,"payment":0.95},
"懒惰":{"participation":0.1,"consensus":1.0,"payment":0.9},
"恶意":{"participation":0.7,"consensus":0.4,"payment":0.4}
}

DTC=np.zeros(NUM_NODES)
OTPC=np.zeros(NUM_NODES)
CC=np.zeros(NUM_NODES)
CSC=np.zeros(NUM_NODES)

history={lam:np.zeros((NUM_NODES,ROUNDS)) for lam in LAM_LIST}

for r in range(ROUNDS):

    for _ in range(PER_ROUND):

        i=np.random.randint(NUM_NODES)
        b=BEHAVIOR[NODE_TYPES[i]]

        if np.random.rand()>b["participation"]:
            continue

        DTC[i]+=1
        CC[i]+=1

        if np.random.rand()<b["consensus"]:
            CSC[i]+=1

        if np.random.rand()<b["payment"]:
            OTPC[i]+=1

    CSR=np.divide(CSC,CC,out=np.zeros_like(CSC),where=CC>0)
    OTPR=np.divide(OTPC,DTC,out=np.zeros_like(OTPC),where=DTC>0)

    R_local=alpha1*CSR+alpha2*OTPR

    w_consensus=CC/(np.max(CC)+1e-9)
    w_transaction=DTC/(np.max(DTC)+1e-9)

    R_global=beta1*(w_consensus*CSR)+beta2*(w_transaction*OTPR)

    for lam in LAM_LIST:
        history[lam][:,r]=lam*R_local+(1-lam)*R_global

plt.figure(figsize=(6,4))

for node_type in ["可信","懒惰","恶意"]:
    for lam in LAM_LIST:

        plt.plot(
            avg_rep(history[lam],node_type),
            linestyle=LINESTYLE[lam],
            linewidth=2
        )

plt.xlabel("迭代次数",fontsize=18)
plt.ylabel("声誉",fontsize=18)
plt.ylim(0,1)
plt.tick_params(labelsize=14)

plt.tight_layout()
plt.show()

# =========================================================
# (c) 可信服务节点声誉
# =========================================================

beta1,beta2=0.5,0.5

BEHAVIOR={
"可信":{"participation":0.9,"success":0.95,"exec_time":(0.8,1.2)},
"懒惰":{"participation":0.1,"success":1.0,"exec_time":(0.9,1.4)},
"恶意":{"participation":0.7,"success":0.4,"exec_time":(1.5,2.5)}
}

TEC=np.zeros(NUM_NODES)
TSC=np.zeros(NUM_NODES)
TET_sum=np.zeros(NUM_NODES)

history={lam:np.zeros((NUM_NODES,ROUNDS)) for lam in LAM_LIST}

for r in range(ROUNDS):

    for _ in range(PER_ROUND):

        i=np.random.randint(NUM_NODES)
        b=BEHAVIOR[NODE_TYPES[i]]

        if np.random.rand()>b["participation"]:
            continue

        TEC[i]+=1

        if np.random.rand()<b["success"]:
            TSC[i]+=1

        TET_sum[i]+=np.random.uniform(*b["exec_time"])

    TSR=np.divide(TSC,TEC,out=np.zeros_like(TSC),where=TEC>0)
    ATET=np.divide(TET_sum,TEC,out=np.zeros_like(TET_sum),where=TEC>0)

    w_task=TEC/(np.max(TEC)+1e-9)
    GTSR=w_task*TSR

    min_atet=np.min(ATET[ATET>0]) if np.any(ATET>0) else 1.0
    GATET=np.divide(min_atet,ATET,out=np.zeros_like(ATET),where=ATET>0)

    R_global=beta1*GTSR+beta2*GATET

    for lam in LAM_LIST:
        history[lam][:,r]=lam*TSR+(1-lam)*R_global

plt.figure(figsize=(6,4))

for node_type in ["可信","懒惰","恶意"]:
    for lam in LAM_LIST:

        plt.plot(
            avg_rep(history[lam],node_type),
            linestyle=LINESTYLE[lam],
            linewidth=2
        )

plt.xlabel("迭代次数",fontsize=18)
plt.ylabel("声誉",fontsize=18)
plt.ylim(0,1)
plt.tick_params(labelsize=14)

plt.tight_layout()
plt.show()

# =========================================================
# legend 单独 figure
# =========================================================

plt.figure(figsize=(6,4))
plt.axis("off")

plt.legend(
    handles,
    labels,
    loc="center",
    ncol=3,
    fontsize=14,
    frameon=False,
    columnspacing=1,
    handlelength=3
)

plt.show()