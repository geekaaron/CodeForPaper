// SPDX-License-Identifier: MIT
pragma solidity ^0.8.11;

import "./RegistrationManagement.sol";

contract StateManagement {
    RegistrationManagement public immutable registry;

    constructor(address _registry) {
        registry = RegistrationManagement(_registry);
    }

    struct RequesterStats {
        uint256 consensusCount;
        uint256 consensusSuccessCount;
        uint256 paymentCount;
        uint256 timelyPaymentCount;
    }
    mapping(address => RequesterStats) public requesterStats;
    address[] public requesterList;
    mapping(address => bool) private requesterExists;

    struct ProviderStats {
        uint256 consensusCount;
        uint256 consensusSuccessCount;
        uint256 deliveryCount;
        uint256 timelyDeliveryCount;
        uint256 totalDataQuality;
        uint256 dataProductCount;
    }
    mapping(address => ProviderStats) public providerStats;
    address[] public providerList;
    mapping(address => bool) private providerExists;

    struct TEEStats {
        uint256 taskCount;
        uint256 taskSuccessCount;
        uint256 totalExecTime;
    }
    mapping(address => TEEStats) public teeStats;
    address[] public teeList;
    mapping(address => bool) private teeExists;

    modifier onlyRole(RegistrationManagement.Role role) {
        require(registry.isRole(msg.sender, role), "RM: unauthorized role");
        _;
    }

    // ==========================================================
    // 请求者逻辑：共识、支付
    // ==========================================================
    function recordRequesterConsensus(bool success)
        external
        onlyRole(RegistrationManagement.Role.Requester)
    {
        RequesterStats storage c = requesterStats[msg.sender];
        if (!requesterExists[msg.sender]) {
            requesterExists[msg.sender] = true;
            requesterList.push(msg.sender);
        }
        c.consensusCount++;
        if (success) c.consensusSuccessCount++;
    }

    function recordRequesterPayment(bool timely)
        external
        onlyRole(RegistrationManagement.Role.Requester)
    {
        RequesterStats storage c = requesterStats[msg.sender];
        if (!requesterExists[msg.sender]) {
            requesterExists[msg.sender] = true;
            requesterList.push(msg.sender);
        }
        c.paymentCount++;
        if (timely) c.timelyPaymentCount++;
    }

    // ==========================================================
    // 提供者逻辑：共识、交付、质量
    // ==========================================================
    function recordProviderConsensus(bool success)
        external
        onlyRole(RegistrationManagement.Role.Provider)
    {
        ProviderStats storage c = providerStats[msg.sender];
        if (!providerExists[msg.sender]) {
            providerExists[msg.sender] = true;
            providerList.push(msg.sender);
        }
        c.consensusCount++;
        if (success) c.consensusSuccessCount++;
    }

    function recordProviderDelivery(bool timely)
        external
        onlyRole(RegistrationManagement.Role.Provider)
    {
        ProviderStats storage c = providerStats[msg.sender];
        if (!providerExists[msg.sender]) {
            providerExists[msg.sender] = true;
            providerList.push(msg.sender);
        }
        c.deliveryCount++;
        if (timely) c.timelyDeliveryCount++;
    }

    function recordProviderDataQuality(uint256 score)
        external
        onlyRole(RegistrationManagement.Role.Provider)
    {
        require(score <= 100, "RM: invalid score");
        ProviderStats storage c = providerStats[msg.sender];
        if (!providerExists[msg.sender]) {
            providerExists[msg.sender] = true;
            providerList.push(msg.sender);
        }
        c.dataProductCount++;
        c.totalDataQuality += score;
    }

    // ==========================================================
    // TEE 节点逻辑：任务执行
    // ==========================================================
    function recordTEETask(bool success, uint256 execTime)
        external
        onlyRole(RegistrationManagement.Role.TEE)
    {
        TEEStats storage c = teeStats[msg.sender];
        if (!teeExists[msg.sender]) {
            teeExists[msg.sender] = true;
            teeList.push(msg.sender);
        }
        c.taskCount++;
        if (success) c.taskSuccessCount++;
        c.totalExecTime += execTime;
    }

    // function clearRequesterRecords() public {
    //     for (uint256 i = 0; i < requesterList.length; i++) {
    //         address a = requesterList[i];
    //         delete requesterStats[a];
    //         requesterExists[a] = false;
    //     }
    //     delete requesterList;
    // }

    // function clearProviderRecords() public {
    //     for (uint256 i = 0; i < providerList.length; i++) {
    //         address a = providerList[i];
    //         delete providerStats[a];
    //         providerExists[a] = false;
    //     }
    //     delete providerList;
    // }

    // function clearTEERecords() public {
    //     for (uint256 i = 0; i < teeList.length; i++) {
    //         address a = teeList[i];
    //         delete teeStats[a];
    //         teeExists[a] = false;
    //     }
    //     delete teeList;
    // }

    // function clearAllRecords() public {
    //     clearRequesterRecords();
    //     clearProviderRecords();
    //     clearTEERecords();
    // }

    // ==========================================================
    // 批量查询接口
    // ==========================================================
    function getAllRequesterStats()
        external
        view
        returns (address[] memory, RequesterStats[] memory)
    {
        uint256 len = requesterList.length;
        RequesterStats[] memory stats = new RequesterStats[](len);
        for (uint256 i = 0; i < len; i++) {
            stats[i] = requesterStats[requesterList[i]];
        }
        return (requesterList, stats);
    }

    function getAllProviderStats()
        external
        view
        returns (address[] memory, ProviderStats[] memory)
    {
        uint256 len = providerList.length;
        ProviderStats[] memory stats = new ProviderStats[](len);
        for (uint256 i = 0; i < len; i++) {
            stats[i] = providerStats[providerList[i]];
        }
        return (providerList, stats);
    }

    function getAllTEEStats()
        external
        view
        returns (address[] memory, TEEStats[] memory)
    {
        uint256 len = teeList.length;
        TEEStats[] memory stats = new TEEStats[](len);
        for (uint256 i = 0; i < len; i++) {
            stats[i] = teeStats[teeList[i]];
        }
        return (teeList, stats);
    }
}
