// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

contract GameManagement {
    struct ProviderInfo {
        string price;
        string amount;
        string reputation;
        string cost;
    }

    struct RequesterInfo {
        string lambda1;
        string lambda2;
        string sensitivity;
    }

    struct TeeInfos {
        bytes raPublicKey;
        bytes ecdsaPublicKey;
        bytes taShdrHash;
        bytes taMemHash;
        bytes teeMemHash;
    }

    struct ConsensusResult {
        bytes hash;
        bytes nonce;
        bytes signature;
    }

    struct GameResult {
        string optimalPrices;
        bytes nonce;
        bytes signature;
    }

    ProviderInfo[] private providersList;
    address[] private providerAddresses;
    mapping(address => bool) private providerExists;

    RequesterInfo[] private requestersList;
    address[] private requesterAddresses;
    mapping(address => bool) private requesterExists;

    address electedTeeAddr;
    address[] private teeAddresses;
    mapping(address => TeeInfos) teeInfos;
    mapping(address => GameResult) gameResults;
    mapping(address => bool) private teeExists;

    address[] private consensusNodeAddresses;
    mapping(address => ConsensusResult) consensusResults;
    mapping(address => bool) private consensusNodeExists;

    function findProviderIndex(address addr) internal view returns (uint) {
        for (uint i = 0; i < providerAddresses.length; i++) {
            if (providerAddresses[i] == addr) {
                return i;
            }
        }
        revert("Provider address not found");
    }

    function findRequesterIndex(address addr) internal view returns (uint) {
        for (uint i = 0; i < requesterAddresses.length; i++) {
            if (requesterAddresses[i] == addr) {
                return i;
            }
        }
        revert("Requester address not found");
    }

    function uploadProvider(string memory price, string memory amount, string memory reputation, string memory cost) public {
        if (providerExists[msg.sender]) {
            uint index = findProviderIndex(msg.sender);
            providersList[index] = ProviderInfo(price, amount, reputation, cost);
        } else {
            providersList.push(ProviderInfo(price, amount, reputation, cost));
            providerAddresses.push(msg.sender);
            providerExists[msg.sender] = true;
        }
    }

    function uploadRequester(string memory lambda1, string memory lambda2, string memory sensitivity ) public {
        if (requesterExists[msg.sender]) {
            uint index = findRequesterIndex(msg.sender);
            requestersList[index] = RequesterInfo(lambda1, lambda2, sensitivity);
        } else {
            requestersList.push(RequesterInfo(lambda1, lambda2, sensitivity));
            requesterAddresses.push(msg.sender);
            requesterExists[msg.sender] = true;
        }
    }

    function uploadTeeInfo(bytes memory raPubKey, bytes memory ecdsaPubKey,
                           bytes memory taShdrHash, bytes memory taMemHash, bytes memory teeMemHash) public {
        require(teeExists[msg.sender] == false, "Tee exists!");
        teeAddresses.push(msg.sender);
        teeInfos[msg.sender] = TeeInfos(raPubKey, ecdsaPubKey, taShdrHash, taMemHash, teeMemHash);
        teeExists[msg.sender] = true;
    }

    function uploadConsensusResult(bytes memory hash, bytes memory nonce, bytes memory signature) public {
        require(consensusNodeExists[msg.sender] == true || msg.sender == electedTeeAddr, "Not a elected consensus node or tee node!");
        consensusResults[msg.sender] = ConsensusResult(hash, nonce, signature);
    }

    function uploadGameResult(string memory optimalPrices, bytes memory nonce, bytes memory signature) public {
        require(electedTeeAddr == msg.sender, "Not this tee!");
        gameResults[msg.sender] = GameResult(optimalPrices, nonce, signature);
    }

    function getProvidersInfo() public view returns (ProviderInfo[] memory) {
        return providersList;
    }

    function getRequestersInfo() public view returns (RequesterInfo[] memory) {
        return requestersList;
    }

    function electConsensus(address nodeAddr) public {
        require(providerExists[nodeAddr] == true || requesterExists[nodeAddr] == true, "Not a valid node address!");
        require(consensusNodeExists[nodeAddr] == false, "Already elected!");
        consensusNodeAddresses.push(msg.sender);
        consensusNodeExists[msg.sender] = true;
    }

    function electTee(address teeAddr) public {
        require(teeExists[teeAddr] == true, "The tee node must be registered!");
        electedTeeAddr = teeAddr;
    }

    function getElectedTeeAddress() public view returns (address) {
        return electedTeeAddr;
    }

    function getElectedTeeInfo() public view returns (TeeInfos memory) {
        return teeInfos[electedTeeAddr];
    }

    function getConsensusResult(address nodeAddr) public view returns (ConsensusResult memory) {
        return consensusResults[nodeAddr];
    }

    function getGameResult(address teeAddr) public view returns (GameResult memory) {
        require(teeExists[teeAddr] == true, "Not a valid tee address!");
        return gameResults[teeAddr];
    }

    function clear() public {
        for (uint i = 0; i < providerAddresses.length; i++) {
            delete providerExists[providerAddresses[i]];
        }
    
        for (uint i = 0; i < requesterAddresses.length; i++) {
            delete requesterExists[requesterAddresses[i]];
        }

        for (uint i = 0; i < consensusNodeAddresses.length; i++) {
            delete consensusNodeExists[consensusNodeAddresses[i]];
            delete consensusResults[consensusNodeAddresses[i]];
        }
    
        for (uint i = 0; i < teeAddresses.length; i++) {
            delete teeInfos[teeAddresses[i]];
            delete teeExists[teeAddresses[i]];
            delete gameResults[teeAddresses[i]];
        }

        delete providersList;
        delete requestersList;
        delete providerAddresses;
        delete requesterAddresses;
        delete consensusNodeAddresses;
        delete teeAddresses;
    }
}

