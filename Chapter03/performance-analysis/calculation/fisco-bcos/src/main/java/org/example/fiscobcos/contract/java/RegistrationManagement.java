package org.example.fiscobcos.contract.java;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.fisco.bcos.sdk.v3.client.Client;
import org.fisco.bcos.sdk.v3.codec.datatypes.Address;
import org.fisco.bcos.sdk.v3.codec.datatypes.Bool;
import org.fisco.bcos.sdk.v3.codec.datatypes.DynamicArray;
import org.fisco.bcos.sdk.v3.codec.datatypes.Event;
import org.fisco.bcos.sdk.v3.codec.datatypes.Function;
import org.fisco.bcos.sdk.v3.codec.datatypes.Type;
import org.fisco.bcos.sdk.v3.codec.datatypes.TypeReference;
import org.fisco.bcos.sdk.v3.codec.datatypes.generated.Uint8;
import org.fisco.bcos.sdk.v3.codec.datatypes.generated.tuples.generated.Tuple1;
import org.fisco.bcos.sdk.v3.codec.datatypes.generated.tuples.generated.Tuple2;
import org.fisco.bcos.sdk.v3.contract.Contract;
import org.fisco.bcos.sdk.v3.crypto.CryptoSuite;
import org.fisco.bcos.sdk.v3.crypto.keypair.CryptoKeyPair;
import org.fisco.bcos.sdk.v3.model.CryptoType;
import org.fisco.bcos.sdk.v3.model.TransactionReceipt;
import org.fisco.bcos.sdk.v3.model.callback.TransactionCallback;
import org.fisco.bcos.sdk.v3.transaction.model.exception.ContractException;

@SuppressWarnings("unchecked")
public class RegistrationManagement extends Contract {
    public static final String[] BINARY_ARRAY = {"608060405234801561001057600080fd5b50600080546001600160a01b0319163317905561083b806100326000396000f3fe608060405234801561001057600080fd5b50600436106100885760003560e01c80639068cbf31161005b5780639068cbf3146100f8578063993746421461011b578063c3c5a5471461014b578063c5e41cf61461016e57600080fd5b806345309ba41461008d5780634aa2289f146100a25780638da5cb5b146100b55780638e96d94e146100e5575b600080fd5b6100a061009b366004610678565b610181565b005b6100a06100b0366004610713565b610358565b6000546100c8906001600160a01b031681565b6040516001600160a01b0390911681526020015b60405180910390f35b6100a06100f3366004610713565b61046c565b61010b610106366004610713565b610571565b60405190151581526020016100dc565b61013e610129366004610746565b60026020526000908152604090205460ff1681565b6040516100dc9190610777565b61010b610159366004610746565b60016020526000908152604090205460ff1681565b6100a061017c366004610746565b6105dc565b6000546001600160a01b031633146101b45760405162461bcd60e51b81526004016101ab9061079f565b60405180910390fd5b60008160038111156101c8576101c8610761565b14156102095760405162461bcd60e51b815260206004820152601060248201526f4e523a20696e76616c696420726f6c6560801b60448201526064016101ab565b60005b8281101561035257600180600086868581811061022b5761022b6107c6565b90506020020160208101906102409190610746565b6001600160a01b03166001600160a01b0316815260200190815260200160002060006101000a81548160ff021916908315150217905550816002600086868581811061028e5761028e6107c6565b90506020020160208101906102a39190610746565b6001600160a01b031681526020810191909152604001600020805460ff191660018360038111156102d6576102d6610761565b02179055508383828181106102ed576102ed6107c6565b90506020020160208101906103029190610746565b6001600160a01b03167f1f63e087b186a95e77f84777db8290b8ac9093ba93592a3c2752d8b789e9c6768360405161033a9190610777565b60405180910390a261034b816107dc565b905061020c565b50505050565b6000546001600160a01b031633146103825760405162461bcd60e51b81526004016101ab9061079f565b600081600381111561039657610396610761565b14156103d75760405162461bcd60e51b815260206004820152601060248201526f4e523a20696e76616c696420726f6c6560801b60448201526064016101ab565b6001600160a01b0382166000908152600160208181526040808420805460ff199081168517909155600290925290922080548493919291169083600381111561042257610422610761565b0217905550816001600160a01b03167f1f63e087b186a95e77f84777db8290b8ac9093ba93592a3c2752d8b789e9c676826040516104609190610777565b60405180910390a25050565b6000546001600160a01b031633146104965760405162461bcd60e51b81526004016101ab9061079f565b6001600160a01b03821660009081526001602052604090205460ff166104fe5760405162461bcd60e51b815260206004820152601760248201527f4e523a206e6f6465206e6f74207265676973746572656400000000000000000060448201526064016101ab565b6001600160a01b0382166000908152600260205260409020805482919060ff1916600183600381111561053357610533610761565b0217905550816001600160a01b03167fc3f8f61911a1537261f77e2703626e158e299a98e341024ecaa26bbd1d884c64826040516104609190610777565b6001600160a01b03821660009081526001602052604081205460ff1680156105d557508160038111156105a6576105a6610761565b6001600160a01b03841660009081526002602052604090205460ff1660038111156105d3576105d3610761565b145b9392505050565b6000546001600160a01b031633146106065760405162461bcd60e51b81526004016101ab9061079f565b6001600160a01b0381166000818152600160209081526040808320805460ff1990811690915560029092528083208054909216909155517fb1864577e4f285436a80ebc833984755393e2450d58622a65fb4fce87ea3573a9190a250565b80356004811061067357600080fd5b919050565b60008060006040848603121561068d57600080fd5b833567ffffffffffffffff808211156106a557600080fd5b818601915086601f8301126106b957600080fd5b8135818111156106c857600080fd5b8760208260051b85010111156106dd57600080fd5b6020928301955093506106f39186019050610664565b90509250925092565b80356001600160a01b038116811461067357600080fd5b6000806040838503121561072657600080fd5b61072f836106fc565b915061073d60208401610664565b90509250929050565b60006020828403121561075857600080fd5b6105d5826106fc565b634e487b7160e01b600052602160045260246000fd5b602081016004831061079957634e487b7160e01b600052602160045260246000fd5b91905290565b6020808252600d908201526c27291d103737ba1037bbb732b960991b604082015260600190565b634e487b7160e01b600052603260045260246000fd5b60006000198214156107fe57634e487b7160e01b600052601160045260246000fd5b506001019056fea2646970667358221220dfaee4286e2886eeadee841f0f331ead84a375d37042d650af0d79e7438260d464736f6c634300080b0033"};

    public static final String BINARY = org.fisco.bcos.sdk.v3.utils.StringUtils.joinAll("", BINARY_ARRAY);

    public static final String[] SM_BINARY_ARRAY = {"608060405234801561001057600080fd5b50600080546001600160a01b03191633179055610842806100326000396000f3fe608060405234801561001057600080fd5b50600436106100885760003560e01c80635089e2c81161005b5780635089e2c814610100578063a49031d21461012b578063b45f8a411461013e578063d3f646011461016e57600080fd5b8063013b4be81461008d57806310aa94c1146100a257806332ad976c146100da578063490d0110146100ed575b600080fd5b6100a061009b36600461067f565b610181565b005b6100c56100b036600461071a565b60016020526000908152604090205460ff1681565b60405190151581526020015b60405180910390f35b6100a06100e8366004610735565b61035a565b6100a06100fb36600461071a565b610470565b600054610113906001600160a01b031681565b6040516001600160a01b0390911681526020016100d1565b6100c5610139366004610735565b6104f9565b61016161014c36600461071a565b60026020526000908152604090205460ff1681565b6040516100d1919061077e565b6100a061017c366004610735565b610564565b6000546001600160a01b031633146101b557604051636381e58960e11b81526004016101ac906107a6565b60405180910390fd5b60008160038111156101c9576101c9610768565b141561020b57604051636381e58960e11b815260206004820152601060248201526f4e523a20696e76616c696420726f6c6560801b60448201526064016101ac565b60005b8281101561035457600180600086868581811061022d5761022d6107cd565b9050602002016020810190610242919061071a565b6001600160a01b03166001600160a01b0316815260200190815260200160002060006101000a81548160ff0219169083151502179055508160026000868685818110610290576102906107cd565b90506020020160208101906102a5919061071a565b6001600160a01b031681526020810191909152604001600020805460ff191660018360038111156102d8576102d8610768565b02179055508383828181106102ef576102ef6107cd565b9050602002016020810190610304919061071a565b6001600160a01b03167fd8dd0898b9cdc1a2392928ea359d8e32bb452a00c10897e560d9c80c756252b58360405161033c919061077e565b60405180910390a261034d816107e3565b905061020e565b50505050565b6000546001600160a01b0316331461038557604051636381e58960e11b81526004016101ac906107a6565b600081600381111561039957610399610768565b14156103db57604051636381e58960e11b815260206004820152601060248201526f4e523a20696e76616c696420726f6c6560801b60448201526064016101ac565b6001600160a01b0382166000908152600160208181526040808420805460ff199081168517909155600290925290922080548493919291169083600381111561042657610426610768565b0217905550816001600160a01b03167fd8dd0898b9cdc1a2392928ea359d8e32bb452a00c10897e560d9c80c756252b582604051610464919061077e565b60405180910390a25050565b6000546001600160a01b0316331461049b57604051636381e58960e11b81526004016101ac906107a6565b6001600160a01b0381166000818152600160209081526040808320805460ff1990811690915560029092528083208054909216909155517f6282070dc75efe2de4bead503379c27a85044a4242393cd35af2643a128f20259190a250565b6001600160a01b03821660009081526001602052604081205460ff16801561055d575081600381111561052e5761052e610768565b6001600160a01b03841660009081526002602052604090205460ff16600381111561055b5761055b610768565b145b9392505050565b6000546001600160a01b0316331461058f57604051636381e58960e11b81526004016101ac906107a6565b6001600160a01b03821660009081526001602052604090205460ff166105f857604051636381e58960e11b815260206004820152601760248201527f4e523a206e6f6465206e6f74207265676973746572656400000000000000000060448201526064016101ac565b6001600160a01b0382166000908152600260205260409020805482919060ff1916600183600381111561062d5761062d610768565b0217905550816001600160a01b03167f8692a8b427cb3501c8a8a190074a01da250e28e80ca2af146ef14f3a6622277882604051610464919061077e565b80356004811061067a57600080fd5b919050565b60008060006040848603121561069457600080fd5b833567ffffffffffffffff808211156106ac57600080fd5b818601915086601f8301126106c057600080fd5b8135818111156106cf57600080fd5b8760208260051b85010111156106e457600080fd5b6020928301955093506106fa918601905061066b565b90509250925092565b80356001600160a01b038116811461067a57600080fd5b60006020828403121561072c57600080fd5b61055d82610703565b6000806040838503121561074857600080fd5b61075183610703565b915061075f6020840161066b565b90509250929050565b63b95aa35560e01b600052602160045260246000fd5b60208101600483106107a05763b95aa35560e01b600052602160045260246000fd5b91905290565b6020808252600d908201526c27291d103737ba1037bbb732b960991b604082015260600190565b63b95aa35560e01b600052603260045260246000fd5b60006000198214156108055763b95aa35560e01b600052601160045260246000fd5b506001019056fea26469706673582212207c163a7b6227f207dec694f309e2db76034944fc8ce74dd8c66b839d79175e0a64736f6c634300080b0033"};

    public static final String SM_BINARY = org.fisco.bcos.sdk.v3.utils.StringUtils.joinAll("", SM_BINARY_ARRAY);

    public static final String[] ABI_ARRAY = {"[{\"inputs\":[],\"stateMutability\":\"nonpayable\",\"type\":\"constructor\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"internalType\":\"address\",\"name\":\"node\",\"type\":\"address\"},{\"indexed\":false,\"internalType\":\"enum RegistrationManagement.Role\",\"name\":\"role\",\"type\":\"uint8\"}],\"name\":\"NodeRegistered\",\"type\":\"event\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"internalType\":\"address\",\"name\":\"node\",\"type\":\"address\"}],\"name\":\"NodeUnregistered\",\"type\":\"event\"},{\"anonymous\":false,\"inputs\":[{\"indexed\":true,\"internalType\":\"address\",\"name\":\"node\",\"type\":\"address\"},{\"indexed\":false,\"internalType\":\"enum RegistrationManagement.Role\",\"name\":\"role\",\"type\":\"uint8\"}],\"name\":\"RoleUpdated\",\"type\":\"event\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":4,\"value\":[0]}],\"inputs\":[{\"internalType\":\"address[]\",\"name\":\"_nodes\",\"type\":\"address[]\"},{\"internalType\":\"enum RegistrationManagement.Role\",\"name\":\"_role\",\"type\":\"uint8\"}],\"name\":\"batchRegister\",\"outputs\":[],\"selector\":[1160813476,20663272],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":3,\"slot\":1,\"value\":[0]}],\"inputs\":[{\"internalType\":\"address\",\"name\":\"\",\"type\":\"address\"}],\"name\":\"isRegistered\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"selector\":[3284510023,279614657],\"stateMutability\":\"view\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0}],\"inputs\":[{\"internalType\":\"address\",\"name\":\"_node\",\"type\":\"address\"},{\"internalType\":\"enum RegistrationManagement.Role\",\"name\":\"_role\",\"type\":\"uint8\"}],\"name\":\"isRole\",\"outputs\":[{\"internalType\":\"bool\",\"name\":\"\",\"type\":\"bool\"}],\"selector\":[2422787059,2760913362],\"stateMutability\":\"view\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":4,\"value\":[0]}],\"inputs\":[],\"name\":\"owner\",\"outputs\":[{\"internalType\":\"address\",\"name\":\"\",\"type\":\"address\"}],\"selector\":[2376452955,1351213768],\"stateMutability\":\"view\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":4,\"value\":[0]}],\"inputs\":[{\"internalType\":\"address\",\"name\":\"_node\",\"type\":\"address\"},{\"internalType\":\"enum RegistrationManagement.Role\",\"name\":\"_role\",\"type\":\"uint8\"}],\"name\":\"registerNode\",\"outputs\":[],\"selector\":[1252141215,850237292],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":3,\"slot\":2,\"value\":[0]}],\"inputs\":[{\"internalType\":\"address\",\"name\":\"\",\"type\":\"address\"}],\"name\":\"roles\",\"outputs\":[{\"internalType\":\"enum RegistrationManagement.Role\",\"name\":\"\",\"type\":\"uint8\"}],\"selector\":[2570536514,3026160193],\"stateMutability\":\"view\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":4,\"value\":[0]}],\"inputs\":[{\"internalType\":\"address\",\"name\":\"_node\",\"type\":\"address\"}],\"name\":\"unregisterNode\",\"outputs\":[],\"selector\":[3320061174,1225589008],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":4,\"value\":[0]}],\"inputs\":[{\"internalType\":\"address\",\"name\":\"_node\",\"type\":\"address\"},{\"internalType\":\"enum RegistrationManagement.Role\",\"name\":\"_newRole\",\"type\":\"uint8\"}],\"name\":\"updateRole\",\"outputs\":[],\"selector\":[2392250702,3556132353],\"stateMutability\":\"nonpayable\",\"type\":\"function\"}]"};

    public static final String ABI = org.fisco.bcos.sdk.v3.utils.StringUtils.joinAll("", ABI_ARRAY);

    public static final String FUNC_BATCHREGISTER = "batchRegister";

    public static final String FUNC_ISREGISTERED = "isRegistered";

    public static final String FUNC_ISROLE = "isRole";

    public static final String FUNC_OWNER = "owner";

    public static final String FUNC_REGISTERNODE = "registerNode";

    public static final String FUNC_ROLES = "roles";

    public static final String FUNC_UNREGISTERNODE = "unregisterNode";

    public static final String FUNC_UPDATEROLE = "updateRole";

    public static final Event NODEREGISTERED_EVENT = new Event("NodeRegistered", 
            Arrays.<TypeReference<?>>asList(new TypeReference<Address>(true) {}, new TypeReference<Uint8>() {}));
    ;

    public static final Event NODEUNREGISTERED_EVENT = new Event("NodeUnregistered", 
            Arrays.<TypeReference<?>>asList(new TypeReference<Address>(true) {}));
    ;

    public static final Event ROLEUPDATED_EVENT = new Event("RoleUpdated", 
            Arrays.<TypeReference<?>>asList(new TypeReference<Address>(true) {}, new TypeReference<Uint8>() {}));
    ;

    protected RegistrationManagement(String contractAddress, Client client,
            CryptoKeyPair credential) {
        super(getBinary(client.getCryptoSuite()), contractAddress, client, credential);
    }

    public static String getBinary(CryptoSuite cryptoSuite) {
        return (cryptoSuite.getCryptoTypeConfig() == CryptoType.ECDSA_TYPE ? BINARY : SM_BINARY);
    }

    public static String getABI() {
        return ABI;
    }

    public List<NodeRegisteredEventResponse> getNodeRegisteredEvents(
            TransactionReceipt transactionReceipt) {
        List<EventValuesWithLog> valueList = extractEventParametersWithLog(NODEREGISTERED_EVENT, transactionReceipt);
        ArrayList<NodeRegisteredEventResponse> responses = new ArrayList<NodeRegisteredEventResponse>(valueList.size());
        for (EventValuesWithLog eventValues : valueList) {
            NodeRegisteredEventResponse typedResponse = new NodeRegisteredEventResponse();
            typedResponse.log = eventValues.getLog();
            typedResponse.node = (String) eventValues.getIndexedValues().get(0).getValue();
            typedResponse.role = (BigInteger) eventValues.getNonIndexedValues().get(0).getValue();
            responses.add(typedResponse);
        }
        return responses;
    }

    public List<NodeUnregisteredEventResponse> getNodeUnregisteredEvents(
            TransactionReceipt transactionReceipt) {
        List<EventValuesWithLog> valueList = extractEventParametersWithLog(NODEUNREGISTERED_EVENT, transactionReceipt);
        ArrayList<NodeUnregisteredEventResponse> responses = new ArrayList<NodeUnregisteredEventResponse>(valueList.size());
        for (EventValuesWithLog eventValues : valueList) {
            NodeUnregisteredEventResponse typedResponse = new NodeUnregisteredEventResponse();
            typedResponse.log = eventValues.getLog();
            typedResponse.node = (String) eventValues.getIndexedValues().get(0).getValue();
            responses.add(typedResponse);
        }
        return responses;
    }

    public List<RoleUpdatedEventResponse> getRoleUpdatedEvents(
            TransactionReceipt transactionReceipt) {
        List<EventValuesWithLog> valueList = extractEventParametersWithLog(ROLEUPDATED_EVENT, transactionReceipt);
        ArrayList<RoleUpdatedEventResponse> responses = new ArrayList<RoleUpdatedEventResponse>(valueList.size());
        for (EventValuesWithLog eventValues : valueList) {
            RoleUpdatedEventResponse typedResponse = new RoleUpdatedEventResponse();
            typedResponse.log = eventValues.getLog();
            typedResponse.node = (String) eventValues.getIndexedValues().get(0).getValue();
            typedResponse.role = (BigInteger) eventValues.getNonIndexedValues().get(0).getValue();
            responses.add(typedResponse);
        }
        return responses;
    }

    public TransactionReceipt batchRegister(List<String> _nodes, BigInteger _role) {
        final Function function = new Function(
                FUNC_BATCHREGISTER, 
                Arrays.<Type>asList(new DynamicArray<Address>(
                        Address.class,
                        org.fisco.bcos.sdk.v3.codec.Utils.typeMap(_nodes, Address.class)),
                new Uint8(_role)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return executeTransaction(function);
    }

    public String batchRegister(List<String> _nodes, BigInteger _role,
            TransactionCallback callback) {
        final Function function = new Function(
                FUNC_BATCHREGISTER, 
                Arrays.<Type>asList(new DynamicArray<Address>(
                        Address.class,
                        org.fisco.bcos.sdk.v3.codec.Utils.typeMap(_nodes, Address.class)),
                new Uint8(_role)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return asyncExecuteTransaction(function, callback);
    }

    public String getSignedTransactionForBatchRegister(List<String> _nodes, BigInteger _role) {
        final Function function = new Function(
                FUNC_BATCHREGISTER, 
                Arrays.<Type>asList(new DynamicArray<Address>(
                        Address.class,
                        org.fisco.bcos.sdk.v3.codec.Utils.typeMap(_nodes, Address.class)),
                new Uint8(_role)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return createSignedTransaction(function);
    }

    public Tuple2<List<String>, BigInteger> getBatchRegisterInput(
            TransactionReceipt transactionReceipt) {
        String data = transactionReceipt.getInput().substring(10);
        final Function function = new Function(FUNC_BATCHREGISTER, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<DynamicArray<Address>>() {}, new TypeReference<Uint8>() {}));
        List<Type> results = this.functionReturnDecoder.decode(data, function.getOutputParameters());
        return new Tuple2<List<String>, BigInteger>(

                convertToNative((List<Address>) results.get(0).getValue()), 
                (BigInteger) results.get(1).getValue()
                );
    }

    public Boolean isRegistered(String param0) throws ContractException {
        final Function function = new Function(FUNC_ISREGISTERED, 
                Arrays.<Type>asList(new Address(param0)),
                Arrays.<TypeReference<?>>asList(new TypeReference<Bool>() {}));
        return executeCallWithSingleValueReturn(function, Boolean.class);
    }

    public Boolean isRole(String _node, BigInteger _role) throws ContractException {
        final Function function = new Function(FUNC_ISROLE, 
                Arrays.<Type>asList(new Address(_node),
                new Uint8(_role)),
                Arrays.<TypeReference<?>>asList(new TypeReference<Bool>() {}));
        return executeCallWithSingleValueReturn(function, Boolean.class);
    }

    public String owner() throws ContractException {
        final Function function = new Function(FUNC_OWNER, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<Address>() {}));
        return executeCallWithSingleValueReturn(function, String.class);
    }

    public TransactionReceipt registerNode(String _node, BigInteger _role) {
        final Function function = new Function(
                FUNC_REGISTERNODE, 
                Arrays.<Type>asList(new Address(_node),
                new Uint8(_role)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return executeTransaction(function);
    }

    public String registerNode(String _node, BigInteger _role, TransactionCallback callback) {
        final Function function = new Function(
                FUNC_REGISTERNODE, 
                Arrays.<Type>asList(new Address(_node),
                new Uint8(_role)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return asyncExecuteTransaction(function, callback);
    }

    public String getSignedTransactionForRegisterNode(String _node, BigInteger _role) {
        final Function function = new Function(
                FUNC_REGISTERNODE, 
                Arrays.<Type>asList(new Address(_node),
                new Uint8(_role)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return createSignedTransaction(function);
    }

    public Tuple2<String, BigInteger> getRegisterNodeInput(TransactionReceipt transactionReceipt) {
        String data = transactionReceipt.getInput().substring(10);
        final Function function = new Function(FUNC_REGISTERNODE, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<Address>() {}, new TypeReference<Uint8>() {}));
        List<Type> results = this.functionReturnDecoder.decode(data, function.getOutputParameters());
        return new Tuple2<String, BigInteger>(

                (String) results.get(0).getValue(), 
                (BigInteger) results.get(1).getValue()
                );
    }

    public BigInteger roles(String param0) throws ContractException {
        final Function function = new Function(FUNC_ROLES, 
                Arrays.<Type>asList(new Address(param0)),
                Arrays.<TypeReference<?>>asList(new TypeReference<Uint8>() {}));
        return executeCallWithSingleValueReturn(function, BigInteger.class);
    }

    public TransactionReceipt unregisterNode(String _node) {
        final Function function = new Function(
                FUNC_UNREGISTERNODE, 
                Arrays.<Type>asList(new Address(_node)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return executeTransaction(function);
    }

    public String unregisterNode(String _node, TransactionCallback callback) {
        final Function function = new Function(
                FUNC_UNREGISTERNODE, 
                Arrays.<Type>asList(new Address(_node)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return asyncExecuteTransaction(function, callback);
    }

    public String getSignedTransactionForUnregisterNode(String _node) {
        final Function function = new Function(
                FUNC_UNREGISTERNODE, 
                Arrays.<Type>asList(new Address(_node)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return createSignedTransaction(function);
    }

    public Tuple1<String> getUnregisterNodeInput(TransactionReceipt transactionReceipt) {
        String data = transactionReceipt.getInput().substring(10);
        final Function function = new Function(FUNC_UNREGISTERNODE, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<Address>() {}));
        List<Type> results = this.functionReturnDecoder.decode(data, function.getOutputParameters());
        return new Tuple1<String>(

                (String) results.get(0).getValue()
                );
    }

    public TransactionReceipt updateRole(String _node, BigInteger _newRole) {
        final Function function = new Function(
                FUNC_UPDATEROLE, 
                Arrays.<Type>asList(new Address(_node),
                new Uint8(_newRole)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return executeTransaction(function);
    }

    public String updateRole(String _node, BigInteger _newRole, TransactionCallback callback) {
        final Function function = new Function(
                FUNC_UPDATEROLE, 
                Arrays.<Type>asList(new Address(_node),
                new Uint8(_newRole)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return asyncExecuteTransaction(function, callback);
    }

    public String getSignedTransactionForUpdateRole(String _node, BigInteger _newRole) {
        final Function function = new Function(
                FUNC_UPDATEROLE, 
                Arrays.<Type>asList(new Address(_node),
                new Uint8(_newRole)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return createSignedTransaction(function);
    }

    public Tuple2<String, BigInteger> getUpdateRoleInput(TransactionReceipt transactionReceipt) {
        String data = transactionReceipt.getInput().substring(10);
        final Function function = new Function(FUNC_UPDATEROLE, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<Address>() {}, new TypeReference<Uint8>() {}));
        List<Type> results = this.functionReturnDecoder.decode(data, function.getOutputParameters());
        return new Tuple2<String, BigInteger>(

                (String) results.get(0).getValue(), 
                (BigInteger) results.get(1).getValue()
                );
    }

    public static RegistrationManagement load(String contractAddress, Client client,
            CryptoKeyPair credential) {
        return new RegistrationManagement(contractAddress, client, credential);
    }

    public static RegistrationManagement deploy(Client client, CryptoKeyPair credential) throws
            ContractException {
        return deploy(RegistrationManagement.class, client, credential, getBinary(client.getCryptoSuite()), getABI(), null, null);
    }

    public static class NodeRegisteredEventResponse {
        public TransactionReceipt.Logs log;

        public String node;

        public BigInteger role;
    }

    public static class NodeUnregisteredEventResponse {
        public TransactionReceipt.Logs log;

        public String node;
    }

    public static class RoleUpdatedEventResponse {
        public TransactionReceipt.Logs log;

        public String node;

        public BigInteger role;
    }
}
