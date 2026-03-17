// SPDX-License-Identifier: MIT
pragma solidity ^0.8.11;

contract RegistrationManagement {
    enum Role {
        None,
        Requester,
        Provider,
        TEE
    }

    address public owner;
    mapping(address => bool) public isRegistered;
    mapping(address => Role) public roles;

    event NodeRegistered(address indexed node, Role role);
    event NodeUnregistered(address indexed node);
    event RoleUpdated(address indexed node, Role role);

    modifier onlyOwner() {
        require(msg.sender == owner, "NR: not owner");
        _;
    }

    constructor() {
        owner = msg.sender;
    }

    function registerNode(address _node, Role _role) external onlyOwner {
        require(_role != Role.None, "NR: invalid role");
        isRegistered[_node] = true;
        roles[_node] = _role;
        emit NodeRegistered(_node, _role);
    }

    function batchRegister(address[] calldata _nodes, Role _role) external onlyOwner {
        require(_role != Role.None, "NR: invalid role");
        for (uint256 i = 0; i < _nodes.length; ++i) {
            isRegistered[_nodes[i]] = true;
            roles[_nodes[i]] = _role;
            emit NodeRegistered(_nodes[i], _role);
        }
    }

    function unregisterNode(address _node) external onlyOwner {
        isRegistered[_node] = false;
        roles[_node] = Role.None;
        emit NodeUnregistered(_node);
    }

    function updateRole(address _node, Role _newRole) external onlyOwner {
        require(isRegistered[_node], "NR: node not registered");
        roles[_node] = _newRole;
        emit RoleUpdated(_node, _newRole);
    }

    function isRole(address _node, Role _role) external view returns (bool) {
        return (isRegistered[_node] && roles[_node] == _role);
    }
}
