// SPDX-License-Identifier: MIT
pragma solidity ^0.8.11;

contract GaStateManagement {
    // ================= Basic (γ, δ, ς, φ) =================
    struct BasicInfo {
        uint256 mem;   // γ
        uint256 cpu;   // δ
        uint256 sensor; // ς
        uint256 bandWidth;      // φ
    }

    BasicInfo[] private basicList;
    address[] private basicAddrs;
    mapping(address => bool) private basicExists;
    mapping(address => uint256) private basicIndex; // addr -> index

    function uploadBasic(uint256 mem, uint256 cpu, uint256 sensor, uint256 bandWidth) public {
        if (basicExists[msg.sender]) {
            uint256 idx = basicIndex[msg.sender];
            basicList[idx] = BasicInfo(mem, cpu, sensor, bandWidth);
        } else {
            basicList.push(BasicInfo(mem, cpu, sensor, bandWidth));
            basicAddrs.push(msg.sender);
            basicIndex[msg.sender] = basicList.length - 1;
            basicExists[msg.sender] = true;
        }
    }

    function getAllBasic() public view
        returns (address[] memory addrs, BasicInfo[] memory infos) {
        return (basicAddrs, basicList);
    }

    function basicCount() public view returns (uint256) {
        return basicAddrs.length;
    }

    // ============== Latest Consensus Indicators (ψ, ρ, Γ, τp, τr) ==============

    struct Indicators {
        uint256 dataQuality;     // ψ 正向
        uint256 consensusEnthusiasm;     // ρ 正向
        uint256 consensusSuccessRate; // Γ 正向（成功率）
        uint256 consensusProccesingLatency;    // τp 负向（处理时延）
        uint256 consensusResponseLatency;    // τr 负向（响应时延）
    }

    Indicators[] private indList;
    address[] private indAddrs;
    mapping(address => bool) private indExists;
    mapping(address => uint256) private indIndex;

    function uploadIndicators(uint256 dataQuality, uint256 consensusEnthusiasm, 
                              uint256 consensusSuccessRate, uint256 consensusProccesingLatency,
                              uint256 consensusResponseLatency) public {
        if (indExists[msg.sender]) {
            uint256 idx = indIndex[msg.sender];
            indList[idx] = Indicators(dataQuality, consensusEnthusiasm, consensusSuccessRate, consensusProccesingLatency, consensusResponseLatency);
        } else {
            indList.push(Indicators(dataQuality, consensusEnthusiasm, consensusSuccessRate, consensusProccesingLatency, consensusResponseLatency));
            indAddrs.push(msg.sender);
            indIndex[msg.sender] = indList.length - 1;
            indExists[msg.sender] = true;
        }
    }

    function getAllIndicators() public view
        returns (address[] memory addrs, Indicators[] memory infos) {
        return (indAddrs, indList);
    }

    function indicatorsCount() public view returns (uint256) {
        return indAddrs.length;
    }

    // ================= Latest Final Reputation (Rt only) =================

    uint256[] private rtList;
    address[] private rtAddrs;
    mapping(address => bool) private rtExists;
    mapping(address => uint256) private rtIndex;

    function uploadFinalReputation(uint256 Rt_1e18) public {
        if (rtExists[msg.sender]) {
            uint256 idx = rtIndex[msg.sender];
            rtList[idx] = Rt_1e18;
        } else {
            rtList.push(Rt_1e18);
            rtAddrs.push(msg.sender);
            rtIndex[msg.sender] = rtList.length - 1;
            rtExists[msg.sender] = true;
        }
    }

    function getAllFinalReputation() public view
        returns (address[] memory addrs, uint256[] memory Rt_1e18) {
        return (rtAddrs, rtList);
    }

    function finalReputationCount() public view returns (uint256) {
        return rtAddrs.length;
    }

    // ================= Optional Clear (for repeated tests) =================
    function clearBasic() public {
        for (uint256 i = 0; i < basicAddrs.length; i++) {
            address a = basicAddrs[i];
            delete basicExists[a];
            delete basicIndex[a];
        }
        delete basicList;
        delete basicAddrs;
    }

    function clearIndicators() public {
        for (uint256 i = 0; i < indAddrs.length; i++) {
            address a = indAddrs[i];
            delete indExists[a];
            delete indIndex[a];
        }
        delete indList;
        delete indAddrs;
    }

    function clearFinalReputation() public {
        for (uint256 i = 0; i < rtAddrs.length; i++) {
            address a = rtAddrs[i];
            delete rtExists[a];
            delete rtIndex[a];
        }
        delete rtList;
        delete rtAddrs;
    }

    function clearAll() public {
        clearBasic();
        clearIndicators();
        clearFinalReputation();
    }
}
