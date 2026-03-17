package org.example.fiscobcos.contract.java;

import java.math.BigInteger;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.fisco.bcos.sdk.v3.client.Client;
import org.fisco.bcos.sdk.v3.codec.datatypes.Address;
import org.fisco.bcos.sdk.v3.codec.datatypes.DynamicArray;
import org.fisco.bcos.sdk.v3.codec.datatypes.Function;
import org.fisco.bcos.sdk.v3.codec.datatypes.StaticStruct;
import org.fisco.bcos.sdk.v3.codec.datatypes.Type;
import org.fisco.bcos.sdk.v3.codec.datatypes.TypeReference;
import org.fisco.bcos.sdk.v3.codec.datatypes.generated.Uint256;
import org.fisco.bcos.sdk.v3.codec.datatypes.generated.tuples.generated.Tuple1;
import org.fisco.bcos.sdk.v3.codec.datatypes.generated.tuples.generated.Tuple2;
import org.fisco.bcos.sdk.v3.codec.datatypes.generated.tuples.generated.Tuple4;
import org.fisco.bcos.sdk.v3.codec.datatypes.generated.tuples.generated.Tuple5;
import org.fisco.bcos.sdk.v3.contract.Contract;
import org.fisco.bcos.sdk.v3.crypto.CryptoSuite;
import org.fisco.bcos.sdk.v3.crypto.keypair.CryptoKeyPair;
import org.fisco.bcos.sdk.v3.model.CryptoType;
import org.fisco.bcos.sdk.v3.model.TransactionReceipt;
import org.fisco.bcos.sdk.v3.model.callback.TransactionCallback;
import org.fisco.bcos.sdk.v3.transaction.model.exception.ContractException;

@SuppressWarnings("unchecked")
public class GaStateManagement extends Contract {
    public static final String[] BINARY_ARRAY = {"608060405234801561001057600080fd5b50610e38806100206000396000f3fe608060405234801561001057600080fd5b50600436106100cf5760003560e01c8063aefe22b21161008c578063c75b3fd811610066578063c75b3fd814610171578063d69ef0fc14610179578063dcac180e1461018c578063ebb689a11461019457600080fd5b8063aefe22b21461014e578063b30e8d8f14610156578063bd7a8a1d1461015e57600080fd5b80630a1eb423146100d457806331e7e02d146100e957806382257cc214610108578063826705141461011a5780639a03acaf14610122578063a6c37f7b14610138575b600080fd5b6100e76100e2366004610b9b565b61019c565b005b6100f16103c3565b6040516100ff929190610c1a565b60405180910390f35b6005545b6040519081526020016100ff565b6100e7610480565b61012a610501565b6040516100ff929190610c71565b6101406105ed565b6040516100ff929190610ce7565b6100e76106d5565b60015461010c565b6100e761016c366004610d59565b610755565b60095461010c565b6100e7610187366004610d72565b61084d565b6100e7610a31565b6100e7610ab1565b3360009081526006602052604090205460ff161561024a573360009081526007602090815260409182902054825160a0810184528881529182018790529181018590526060810184905260808101839052600480548390811061020157610201610da4565b90600052602060002090600502016000820151816000015560208201518160010155604082015181600201556060820151816003015560808201518160040155905050506103bc565b6040805160a08101825286815260208101868152918101858152606082018581526080830185815260048054600181810183556000838152965160059283027f8a35acfbc15ff81a39ae7d344fd709f28e8600b4aa8c65c6b64bfe7fe36bd19b81019190915597517f8a35acfbc15ff81a39ae7d344fd709f28e8600b4aa8c65c6b64bfe7fe36bd19c89015594517f8a35acfbc15ff81a39ae7d344fd709f28e8600b4aa8c65c6b64bfe7fe36bd19d88015592517f8a35acfbc15ff81a39ae7d344fd709f28e8600b4aa8c65c6b64bfe7fe36bd19e87015590517f8a35acfbc15ff81a39ae7d344fd709f28e8600b4aa8c65c6b64bfe7fe36bd19f909501949094558054808301825592527f036b6384b5eca791c62761152d0c79bb0604c104a5fb6f4eb0703f3154bb3db090910180546001600160a01b0319163317905590546103959190610dd0565b336000908152600760209081526040808320939093556006905220805460ff191660011790555b5050505050565b606080600960088180548060200260200160405190810160405280929190818152602001828054801561041f57602002820191906000526020600020905b81546001600160a01b03168152600190910190602001808311610401575b505050505091508080548060200260200160405190810160405280929190818152602001828054801561047157602002820191906000526020600020905b81548152602001906001019080831161045d575b50505050509050915091509091565b60005b6001548110156104e7576000600182815481106104a2576104a2610da4565b60009182526020808320909101546001600160a01b03168252600281526040808320805460ff19169055600390915281205550806104df81610de7565b915050610483565b506104f3600080610ac9565b6104ff60016000610aea565b565b606080600160008180548060200260200160405190810160405280929190818152602001828054801561055d57602002820191906000526020600020905b81546001600160a01b0316815260019091019060200180831161053f575b5050505050915080805480602002602001604051908101604052809291908181526020016000905b828210156105df578382906000526020600020906004020160405180608001604052908160008201548152602001600182015481526020016002820154815260200160038201548152505081526020019060010190610585565b505050509050915091509091565b606080600560048180548060200260200160405190810160405280929190818152602001828054801561064957602002820191906000526020600020905b81546001600160a01b0316815260019091019060200180831161062b575b5050505050915080805480602002602001604051908101604052809291908181526020016000905b828210156105df57838290600052602060002090600502016040518060a00160405290816000820154815260200160018201548152602001600282015481526020016003820154815260200160048201548152505081526020019060010190610671565b60005b60055481101561073c576000600582815481106106f7576106f7610da4565b60009182526020808320909101546001600160a01b03168252600681526040808320805460ff191690556007909152812055508061073481610de7565b9150506106d8565b5061074960046000610b08565b6104ff60056000610aea565b336000908152600a602052604090205460ff16156107a457336000908152600b6020526040902054600880548391908390811061079457610794610da4565b6000918252602090912001555050565b60088054600181810183557ff3f7a9fe364faab93b216da50a3214154f22a0a2b415b23a84c8169e8b636ee39091018390556009805480830182556000919091527f6e1540171b6c0c960b71a7020d9f60077f6af931a8bbf590da0223dacf75c7af0180546001600160a01b0319163317905590546108239190610dd0565b336000908152600b6020908152604080832093909355600a905220805460ff191660011790555b50565b3360009081526002602052604090205460ff16156108ea57336000908152600360209081526040808320548151608081018352888152928301879052908201859052606082018490528254909290839081106108ab576108ab610da4565b90600052602060002090600402016000820151816000015560208201518160010155604082015181600201556060820151816003015590505050610a2b565b6040805160808101825285815260208101858152918101848152606082018481526000805460018181018355828052945160049091027f290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e56381019190915594517f290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e56486015591517f290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e565850155517f290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e56690930192909255805480820182558183527fb10e2d527612073b26eecdfd717e6a320cf44b4afac2b0732d9fcbe2b7fa0cf60180546001600160a01b031916331790559054610a049190610dd0565b336000908152600360209081526040808320939093556002905220805460ff191660011790555b50505050565b60005b600954811015610a9857600060098281548110610a5357610a53610da4565b60009182526020808320909101546001600160a01b03168252600a81526040808320805460ff19169055600b9091528120555080610a9081610de7565b915050610a34565b50610aa560086000610aea565b6104ff60096000610aea565b610ab9610480565b610ac16106d5565b6104ff610a31565b508054600082556004029060005260206000209081019061084a9190610b29565b508054600082559060005260206000209081019061084a9190610b56565b508054600082556005029060005260206000209081019061084a9190610b6b565b5b80821115610b5257600080825560018201819055600282018190556003820155600401610b2a565b5090565b5b80821115610b525760008155600101610b57565b5b80821115610b525760008082556001820181905560028201819055600382018190556004820155600501610b6c565b600080600080600060a08688031215610bb357600080fd5b505083359560208501359550604085013594606081013594506080013592509050565b600081518084526020808501945080840160005b83811015610c0f5781516001600160a01b031687529582019590820190600101610bea565b509495945050505050565b604081526000610c2d6040830185610bd6565b82810360208481019190915284518083528582019282019060005b81811015610c6457845183529383019391830191600101610c48565b5090979650505050505050565b60006040808352610c8481840186610bd6565b83810360208581019190915285518083528682019282019060005b81811015610cd957845180518452848101518585015286810151878501526060908101519084015293830193608090920191600101610c9f565b509098975050505050505050565b60006040808352610cfa81840186610bd6565b83810360208581019190915285518083528682019282019060005b81811015610cd9578451805184528481015185850152868101518785015260608082015190850152608090810151908401529383019360a090920191600101610d15565b600060208284031215610d6b57600080fd5b5035919050565b60008060008060808587031215610d8857600080fd5b5050823594602084013594506040840135936060013592509050565b634e487b7160e01b600052603260045260246000fd5b634e487b7160e01b600052601160045260246000fd5b600082821015610de257610de2610dba565b500390565b6000600019821415610dfb57610dfb610dba565b506001019056fea2646970667358221220d67f6337dce59623d2ec923c69c2ce150d01b26a26775052e92cd7b1c1dd261c64736f6c634300080b0033"};

    public static final String BINARY = org.fisco.bcos.sdk.v3.utils.StringUtils.joinAll("", BINARY_ARRAY);

    public static final String[] SM_BINARY_ARRAY = {"608060405234801561001057600080fd5b50610e34806100206000396000f3fe608060405234801561001057600080fd5b50600436106100cf5760003560e01c8063820f93c01161008c578063c9963af011610066578063c9963af014610151578063d69a705f14610167578063de807b141461016f578063e29da7481461018257600080fd5b8063820f93c01461012b578063866867dd14610133578063a7798d7c1461013b57600080fd5b80630b6f95bd146100d457806314af0f6b146100e957806336a3f9d8146100fc578063454ded15146101045780634fad282f1461011b578063620e5d5214610123575b600080fd5b6100e76100e2366004610b97565b610198565b005b6100e76100f7366004610bb0565b610290565b6100e76104b7565b6009545b6040519081526020015b60405180910390f35b6100e7610539565b6100e76105b8565b600554610108565b600154610108565b610143610638565b604051610112929190610c2f565b610159610724565b604051610112929190610ca5565b6100e76107e1565b6100e761017d366004610cfc565b6107f9565b61018a6109dd565b604051610112929190610d2e565b336000908152600a602052604090205460ff16156101e757336000908152600b602052604090205460088054839190839081106101d7576101d7610da0565b6000918252602090912001555050565b60088054600181810183557f6a1365a0cb786d44080e1ebfcbb8ba67e86786ea4a84c682acfac2d417bea72a9091018390556009805480830182556000919091527f78c3693317817d4a9aac2cfdb84a7bb66682fc973735e71b1938aa08a445c6840180546001600160a01b0319163317905590546102669190610dcc565b336000908152600b6020908152604080832093909355600a905220805460ff191660011790555b50565b3360009081526006602052604090205460ff161561033e573360009081526007602090815260409182902054825160a081018452888152918201879052918101859052606081018490526080810183905260048054839081106102f5576102f5610da0565b90600052602060002090600502016000820151816000015560208201518160010155604082015181600201556060820151816003015560808201518160040155905050506104b0565b6040805160a08101825286815260208101868152918101858152606082018581526080830185815260048054600181810183556000838152965160059283027ff6edf6853d0bfb8a1466d7212ac8f73c9fb8e1c4257bb19a3f13f12f35a67fe181019190915597517ff6edf6853d0bfb8a1466d7212ac8f73c9fb8e1c4257bb19a3f13f12f35a67fe289015594517ff6edf6853d0bfb8a1466d7212ac8f73c9fb8e1c4257bb19a3f13f12f35a67fe388015592517ff6edf6853d0bfb8a1466d7212ac8f73c9fb8e1c4257bb19a3f13f12f35a67fe487015590517ff6edf6853d0bfb8a1466d7212ac8f73c9fb8e1c4257bb19a3f13f12f35a67fe5909501949094558054808301825592527f7007368c9e4bf4eb4693ad76ae78762b6c0e97760aa7ac9baa975fe484d9a02990910180546001600160a01b0319163317905590546104899190610dcc565b336000908152600760209081526040808320939093556006905220805460ff191660011790555b5050505050565b60005b60095481101561051e576000600982815481106104d9576104d9610da0565b60009182526020808320909101546001600160a01b03168252600a81526040808320805460ff19169055600b909152812055508061051681610de3565b9150506104ba565b5061052b60086000610ac5565b61053760096000610ac5565b565b60005b6001548110156105a05760006001828154811061055b5761055b610da0565b60009182526020808320909101546001600160a01b03168252600281526040808320805460ff191690556003909152812055508061059881610de3565b91505061053c565b506105ac600080610ae3565b61053760016000610ac5565b60005b60055481101561061f576000600582815481106105da576105da610da0565b60009182526020808320909101546001600160a01b03168252600681526040808320805460ff191690556007909152812055508061061781610de3565b9150506105bb565b5061062c60046000610b04565b61053760056000610ac5565b606080600160008180548060200260200160405190810160405280929190818152602001828054801561069457602002820191906000526020600020905b81546001600160a01b03168152600190910190602001808311610676575b5050505050915080805480602002602001604051908101604052809291908181526020016000905b828210156107165783829060005260206000209060040201604051806080016040529081600082015481526020016001820154815260200160028201548152602001600382015481525050815260200190600101906106bc565b505050509050915091509091565b606080600960088180548060200260200160405190810160405280929190818152602001828054801561078057602002820191906000526020600020905b81546001600160a01b03168152600190910190602001808311610762575b50505050509150808054806020026020016040519081016040528092919081815260200182805480156107d257602002820191906000526020600020905b8154815260200190600101908083116107be575b50505050509050915091509091565b6107e9610539565b6107f16105b8565b6105376104b7565b3360009081526002602052604090205460ff1615610896573360009081526003602090815260408083205481516080810183528881529283018790529082018590526060820184905282549092908390811061085757610857610da0565b906000526020600020906004020160008201518160000155602082015181600101556040820151816002015560608201518160030155905050506109d7565b6040805160808101825285815260208101858152918101848152606082018481526000805460018181018355828052945160049091027fe0bab8f4d8172ba245190d13c94117e93b82166c25b2b69883350c192c90514081019190915594517fe0bab8f4d8172ba245190d13c94117e93b82166c25b2b69883350c192c90514186015591517fe0bab8f4d8172ba245190d13c94117e93b82166c25b2b69883350c192c905142850155517fe0bab8f4d8172ba245190d13c94117e93b82166c25b2b69883350c192c90514390930192909255805480820182558183527f801412028db4471413ca17a15806a4f6857ca4e7c2783885b524c92c7efcdc980180546001600160a01b0319163317905590546109b09190610dcc565b336000908152600360209081526040808320939093556002905220805460ff191660011790555b50505050565b6060806005600481805480602002602001604051908101604052809291908181526020018280548015610a3957602002820191906000526020600020905b81546001600160a01b03168152600190910190602001808311610a1b575b5050505050915080805480602002602001604051908101604052809291908181526020016000905b8282101561071657838290600052602060002090600502016040518060a00160405290816000820154815260200160018201548152602001600282015481526020016003820154815260200160048201548152505081526020019060010190610a61565b508054600082559060005260206000209081019061028d9190610b25565b508054600082556004029060005260206000209081019061028d9190610b3e565b508054600082556005029060005260206000209081019061028d9190610b67565b5b80821115610b3a5760008155600101610b26565b5090565b5b80821115610b3a57600080825560018201819055600282018190556003820155600401610b3f565b5b80821115610b3a5760008082556001820181905560028201819055600382018190556004820155600501610b68565b600060208284031215610ba957600080fd5b5035919050565b600080600080600060a08688031215610bc857600080fd5b505083359560208501359550604085013594606081013594506080013592509050565b600081518084526020808501945080840160005b83811015610c245781516001600160a01b031687529582019590820190600101610bff565b509495945050505050565b60006040808352610c4281840186610beb565b83810360208581019190915285518083528682019282019060005b81811015610c9757845180518452848101518585015286810151878501526060908101519084015293830193608090920191600101610c5d565b509098975050505050505050565b604081526000610cb86040830185610beb565b82810360208481019190915284518083528582019282019060005b81811015610cef57845183529383019391830191600101610cd3565b5090979650505050505050565b60008060008060808587031215610d1257600080fd5b5050823594602084013594506040840135936060013592509050565b60006040808352610d4181840186610beb565b83810360208581019190915285518083528682019282019060005b81811015610c97578451805184528481015185850152868101518785015260608082015190850152608090810151908401529383019360a090920191600101610d5c565b63b95aa35560e01b600052603260045260246000fd5b63b95aa35560e01b600052601160045260246000fd5b600082821015610dde57610dde610db6565b500390565b6000600019821415610df757610df7610db6565b506001019056fea26469706673582212200efb2e934082d948955b90d5049056be31a6221f01608216babbbd4e6bc0868464736f6c634300080b0033"};

    public static final String SM_BINARY = org.fisco.bcos.sdk.v3.utils.StringUtils.joinAll("", SM_BINARY_ARRAY);

    public static final String[] ABI_ARRAY = {"[{\"conflictFields\":[{\"kind\":4,\"value\":[1]}],\"inputs\":[],\"name\":\"basicCount\",\"outputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"selector\":[3004075407,2254989277],\"stateMutability\":\"view\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":4,\"value\":[0]},{\"kind\":4,\"value\":[1]},{\"kind\":4,\"value\":[4]},{\"kind\":4,\"value\":[5]},{\"kind\":4,\"value\":[8]},{\"kind\":4,\"value\":[9]}],\"inputs\":[],\"name\":\"clearAll\",\"outputs\":[],\"selector\":[3954608545,3600445535],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":4,\"value\":[0]},{\"kind\":4,\"value\":[1]}],\"inputs\":[],\"name\":\"clearBasic\",\"outputs\":[],\"selector\":[2187789588,1336748079],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":4,\"value\":[8]},{\"kind\":4,\"value\":[9]}],\"inputs\":[],\"name\":\"clearFinalReputation\",\"outputs\":[],\"selector\":[3702265870,916715992],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":4,\"value\":[4]},{\"kind\":4,\"value\":[5]}],\"inputs\":[],\"name\":\"clearIndicators\",\"outputs\":[],\"selector\":[2935890610,1645108562],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":4,\"value\":[9]}],\"inputs\":[],\"name\":\"finalReputationCount\",\"outputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"selector\":[3344646104,1162734869],\"stateMutability\":\"view\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":4,\"value\":[0]},{\"kind\":4,\"value\":[1]}],\"inputs\":[],\"name\":\"getAllBasic\",\"outputs\":[{\"internalType\":\"address[]\",\"name\":\"addrs\",\"type\":\"address[]\"},{\"components\":[{\"internalType\":\"uint256\",\"name\":\"mem\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"cpu\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"sensor\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"bandWidth\",\"type\":\"uint256\"}],\"internalType\":\"struct GaStateManagement.BasicInfo[]\",\"name\":\"infos\",\"type\":\"tuple[]\"}],\"selector\":[2583932079,2809761148],\"stateMutability\":\"view\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":4,\"value\":[8]},{\"kind\":4,\"value\":[9]}],\"inputs\":[],\"name\":\"getAllFinalReputation\",\"outputs\":[{\"internalType\":\"address[]\",\"name\":\"addrs\",\"type\":\"address[]\"},{\"internalType\":\"uint256[]\",\"name\":\"Rt_1e18\",\"type\":\"uint256[]\"}],\"selector\":[837279789,3382065904],\"stateMutability\":\"view\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":4,\"value\":[4]},{\"kind\":4,\"value\":[5]}],\"inputs\":[],\"name\":\"getAllIndicators\",\"outputs\":[{\"internalType\":\"address[]\",\"name\":\"addrs\",\"type\":\"address[]\"},{\"components\":[{\"internalType\":\"uint256\",\"name\":\"dataQuality\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"consensusEnthusiasm\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"consensusSuccessRate\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"consensusProccesingLatency\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"consensusResponseLatency\",\"type\":\"uint256\"}],\"internalType\":\"struct GaStateManagement.Indicators[]\",\"name\":\"infos\",\"type\":\"tuple[]\"}],\"selector\":[2797830011,3801982792],\"stateMutability\":\"view\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":4,\"value\":[5]}],\"inputs\":[],\"name\":\"indicatorsCount\",\"outputs\":[{\"internalType\":\"uint256\",\"name\":\"\",\"type\":\"uint256\"}],\"selector\":[2183494850,2182058944],\"stateMutability\":\"view\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":2,\"slot\":2,\"value\":[0]},{\"kind\":2,\"slot\":3,\"value\":[0]},{\"kind\":4,\"value\":[0]},{\"kind\":4,\"value\":[1]}],\"inputs\":[{\"internalType\":\"uint256\",\"name\":\"mem\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"cpu\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"sensor\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"bandWidth\",\"type\":\"uint256\"}],\"name\":\"uploadBasic\",\"outputs\":[],\"selector\":[3600740604,3732962068],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":2,\"slot\":10,\"value\":[0]},{\"kind\":2,\"slot\":11,\"value\":[0]},{\"kind\":4,\"value\":[8]},{\"kind\":4,\"value\":[9]}],\"inputs\":[{\"internalType\":\"uint256\",\"name\":\"Rt_1e18\",\"type\":\"uint256\"}],\"name\":\"uploadFinalReputation\",\"outputs\":[],\"selector\":[3178924573,191862205],\"stateMutability\":\"nonpayable\",\"type\":\"function\"},{\"conflictFields\":[{\"kind\":0},{\"kind\":2,\"slot\":6,\"value\":[0]},{\"kind\":2,\"slot\":7,\"value\":[0]},{\"kind\":4,\"value\":[4]},{\"kind\":4,\"value\":[5]}],\"inputs\":[{\"internalType\":\"uint256\",\"name\":\"dataQuality\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"consensusEnthusiasm\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"consensusSuccessRate\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"consensusProccesingLatency\",\"type\":\"uint256\"},{\"internalType\":\"uint256\",\"name\":\"consensusResponseLatency\",\"type\":\"uint256\"}],\"name\":\"uploadIndicators\",\"outputs\":[],\"selector\":[169784355,347017067],\"stateMutability\":\"nonpayable\",\"type\":\"function\"}]"};

    public static final String ABI = org.fisco.bcos.sdk.v3.utils.StringUtils.joinAll("", ABI_ARRAY);

    public static final String FUNC_BASICCOUNT = "basicCount";

    public static final String FUNC_CLEARALL = "clearAll";

    public static final String FUNC_CLEARBASIC = "clearBasic";

    public static final String FUNC_CLEARFINALREPUTATION = "clearFinalReputation";

    public static final String FUNC_CLEARINDICATORS = "clearIndicators";

    public static final String FUNC_FINALREPUTATIONCOUNT = "finalReputationCount";

    public static final String FUNC_GETALLBASIC = "getAllBasic";

    public static final String FUNC_GETALLFINALREPUTATION = "getAllFinalReputation";

    public static final String FUNC_GETALLINDICATORS = "getAllIndicators";

    public static final String FUNC_INDICATORSCOUNT = "indicatorsCount";

    public static final String FUNC_UPLOADBASIC = "uploadBasic";

    public static final String FUNC_UPLOADFINALREPUTATION = "uploadFinalReputation";

    public static final String FUNC_UPLOADINDICATORS = "uploadIndicators";

    protected GaStateManagement(String contractAddress, Client client, CryptoKeyPair credential) {
        super(getBinary(client.getCryptoSuite()), contractAddress, client, credential);
    }

    public static String getBinary(CryptoSuite cryptoSuite) {
        return (cryptoSuite.getCryptoTypeConfig() == CryptoType.ECDSA_TYPE ? BINARY : SM_BINARY);
    }

    public static String getABI() {
        return ABI;
    }

    public BigInteger basicCount() throws ContractException {
        final Function function = new Function(FUNC_BASICCOUNT, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<Uint256>() {}));
        return executeCallWithSingleValueReturn(function, BigInteger.class);
    }

    public TransactionReceipt clearAll() {
        final Function function = new Function(
                FUNC_CLEARALL, 
                Arrays.<Type>asList(), 
                Collections.<TypeReference<?>>emptyList(), 0);
        return executeTransaction(function);
    }

    public String clearAll(TransactionCallback callback) {
        final Function function = new Function(
                FUNC_CLEARALL, 
                Arrays.<Type>asList(), 
                Collections.<TypeReference<?>>emptyList(), 0);
        return asyncExecuteTransaction(function, callback);
    }

    public String getSignedTransactionForClearAll() {
        final Function function = new Function(
                FUNC_CLEARALL, 
                Arrays.<Type>asList(), 
                Collections.<TypeReference<?>>emptyList(), 0);
        return createSignedTransaction(function);
    }

    public TransactionReceipt clearBasic() {
        final Function function = new Function(
                FUNC_CLEARBASIC, 
                Arrays.<Type>asList(), 
                Collections.<TypeReference<?>>emptyList(), 0);
        return executeTransaction(function);
    }

    public String clearBasic(TransactionCallback callback) {
        final Function function = new Function(
                FUNC_CLEARBASIC, 
                Arrays.<Type>asList(), 
                Collections.<TypeReference<?>>emptyList(), 0);
        return asyncExecuteTransaction(function, callback);
    }

    public String getSignedTransactionForClearBasic() {
        final Function function = new Function(
                FUNC_CLEARBASIC, 
                Arrays.<Type>asList(), 
                Collections.<TypeReference<?>>emptyList(), 0);
        return createSignedTransaction(function);
    }

    public TransactionReceipt clearFinalReputation() {
        final Function function = new Function(
                FUNC_CLEARFINALREPUTATION, 
                Arrays.<Type>asList(), 
                Collections.<TypeReference<?>>emptyList(), 0);
        return executeTransaction(function);
    }

    public String clearFinalReputation(TransactionCallback callback) {
        final Function function = new Function(
                FUNC_CLEARFINALREPUTATION, 
                Arrays.<Type>asList(), 
                Collections.<TypeReference<?>>emptyList(), 0);
        return asyncExecuteTransaction(function, callback);
    }

    public String getSignedTransactionForClearFinalReputation() {
        final Function function = new Function(
                FUNC_CLEARFINALREPUTATION, 
                Arrays.<Type>asList(), 
                Collections.<TypeReference<?>>emptyList(), 0);
        return createSignedTransaction(function);
    }

    public TransactionReceipt clearIndicators() {
        final Function function = new Function(
                FUNC_CLEARINDICATORS, 
                Arrays.<Type>asList(), 
                Collections.<TypeReference<?>>emptyList(), 0);
        return executeTransaction(function);
    }

    public String clearIndicators(TransactionCallback callback) {
        final Function function = new Function(
                FUNC_CLEARINDICATORS, 
                Arrays.<Type>asList(), 
                Collections.<TypeReference<?>>emptyList(), 0);
        return asyncExecuteTransaction(function, callback);
    }

    public String getSignedTransactionForClearIndicators() {
        final Function function = new Function(
                FUNC_CLEARINDICATORS, 
                Arrays.<Type>asList(), 
                Collections.<TypeReference<?>>emptyList(), 0);
        return createSignedTransaction(function);
    }

    public BigInteger finalReputationCount() throws ContractException {
        final Function function = new Function(FUNC_FINALREPUTATIONCOUNT, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<Uint256>() {}));
        return executeCallWithSingleValueReturn(function, BigInteger.class);
    }

    public Tuple2<List<String>, List<GaStateManagement.BasicInfo>> getAllBasic() throws
            ContractException {
        final Function function = new Function(FUNC_GETALLBASIC, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<DynamicArray<Address>>() {}, new TypeReference<DynamicArray<GaStateManagement.BasicInfo>>() {}));
        List<Type> results = executeCallWithMultipleValueReturn(function);
        return new Tuple2<List<String>, List<GaStateManagement.BasicInfo>>(
                convertToNative((List<Address>) results.get(0).getValue()), 
                (List<GaStateManagement.BasicInfo>) results.get(1).getValue());
    }

    public Tuple2<List<String>, List<BigInteger>> getAllFinalReputation() throws ContractException {
        final Function function = new Function(FUNC_GETALLFINALREPUTATION, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<DynamicArray<Address>>() {}, new TypeReference<DynamicArray<Uint256>>() {}));
        List<Type> results = executeCallWithMultipleValueReturn(function);
        return new Tuple2<List<String>, List<BigInteger>>(
                convertToNative((List<Address>) results.get(0).getValue()), 
                convertToNative((List<Uint256>) results.get(1).getValue()));
    }

    public Tuple2<List<String>, List<GaStateManagement.Indicators>> getAllIndicators()
            throws ContractException {
        final Function function = new Function(FUNC_GETALLINDICATORS, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<DynamicArray<Address>>() {}, new TypeReference<DynamicArray<GaStateManagement.Indicators>>() {}));
        List<Type> results = executeCallWithMultipleValueReturn(function);
        return new Tuple2<List<String>, List<GaStateManagement.Indicators>>(
                convertToNative((List<Address>) results.get(0).getValue()), 
                (List<GaStateManagement.Indicators>) results.get(1).getValue());
    }

    public BigInteger indicatorsCount() throws ContractException {
        final Function function = new Function(FUNC_INDICATORSCOUNT, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<Uint256>() {}));
        return executeCallWithSingleValueReturn(function, BigInteger.class);
    }

    public TransactionReceipt uploadBasic(BigInteger mem, BigInteger cpu, BigInteger sensor,
            BigInteger bandWidth) {
        final Function function = new Function(
                FUNC_UPLOADBASIC, 
                Arrays.<Type>asList(new Uint256(mem),
                new Uint256(cpu),
                new Uint256(sensor),
                new Uint256(bandWidth)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return executeTransaction(function);
    }

    public String uploadBasic(BigInteger mem, BigInteger cpu, BigInteger sensor,
            BigInteger bandWidth, TransactionCallback callback) {
        final Function function = new Function(
                FUNC_UPLOADBASIC, 
                Arrays.<Type>asList(new Uint256(mem),
                new Uint256(cpu),
                new Uint256(sensor),
                new Uint256(bandWidth)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return asyncExecuteTransaction(function, callback);
    }

    public String getSignedTransactionForUploadBasic(BigInteger mem, BigInteger cpu,
            BigInteger sensor, BigInteger bandWidth) {
        final Function function = new Function(
                FUNC_UPLOADBASIC, 
                Arrays.<Type>asList(new Uint256(mem),
                new Uint256(cpu),
                new Uint256(sensor),
                new Uint256(bandWidth)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return createSignedTransaction(function);
    }

    public Tuple4<BigInteger, BigInteger, BigInteger, BigInteger> getUploadBasicInput(
            TransactionReceipt transactionReceipt) {
        String data = transactionReceipt.getInput().substring(10);
        final Function function = new Function(FUNC_UPLOADBASIC, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<Uint256>() {}, new TypeReference<Uint256>() {}, new TypeReference<Uint256>() {}, new TypeReference<Uint256>() {}));
        List<Type> results = this.functionReturnDecoder.decode(data, function.getOutputParameters());
        return new Tuple4<BigInteger, BigInteger, BigInteger, BigInteger>(

                (BigInteger) results.get(0).getValue(), 
                (BigInteger) results.get(1).getValue(), 
                (BigInteger) results.get(2).getValue(), 
                (BigInteger) results.get(3).getValue()
                );
    }

    public TransactionReceipt uploadFinalReputation(BigInteger Rt_1e18) {
        final Function function = new Function(
                FUNC_UPLOADFINALREPUTATION, 
                Arrays.<Type>asList(new Uint256(Rt_1e18)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return executeTransaction(function);
    }

    public String uploadFinalReputation(BigInteger Rt_1e18, TransactionCallback callback) {
        final Function function = new Function(
                FUNC_UPLOADFINALREPUTATION, 
                Arrays.<Type>asList(new Uint256(Rt_1e18)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return asyncExecuteTransaction(function, callback);
    }

    public String getSignedTransactionForUploadFinalReputation(BigInteger Rt_1e18) {
        final Function function = new Function(
                FUNC_UPLOADFINALREPUTATION, 
                Arrays.<Type>asList(new Uint256(Rt_1e18)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return createSignedTransaction(function);
    }

    public Tuple1<BigInteger> getUploadFinalReputationInput(TransactionReceipt transactionReceipt) {
        String data = transactionReceipt.getInput().substring(10);
        final Function function = new Function(FUNC_UPLOADFINALREPUTATION, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<Uint256>() {}));
        List<Type> results = this.functionReturnDecoder.decode(data, function.getOutputParameters());
        return new Tuple1<BigInteger>(

                (BigInteger) results.get(0).getValue()
                );
    }

    public TransactionReceipt uploadIndicators(BigInteger dataQuality,
            BigInteger consensusEnthusiasm, BigInteger consensusSuccessRate,
            BigInteger consensusProccesingLatency, BigInteger consensusResponseLatency) {
        final Function function = new Function(
                FUNC_UPLOADINDICATORS, 
                Arrays.<Type>asList(new Uint256(dataQuality),
                new Uint256(consensusEnthusiasm),
                new Uint256(consensusSuccessRate),
                new Uint256(consensusProccesingLatency),
                new Uint256(consensusResponseLatency)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return executeTransaction(function);
    }

    public String uploadIndicators(BigInteger dataQuality, BigInteger consensusEnthusiasm,
            BigInteger consensusSuccessRate, BigInteger consensusProccesingLatency,
            BigInteger consensusResponseLatency, TransactionCallback callback) {
        final Function function = new Function(
                FUNC_UPLOADINDICATORS, 
                Arrays.<Type>asList(new Uint256(dataQuality),
                new Uint256(consensusEnthusiasm),
                new Uint256(consensusSuccessRate),
                new Uint256(consensusProccesingLatency),
                new Uint256(consensusResponseLatency)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return asyncExecuteTransaction(function, callback);
    }

    public String getSignedTransactionForUploadIndicators(BigInteger dataQuality,
            BigInteger consensusEnthusiasm, BigInteger consensusSuccessRate,
            BigInteger consensusProccesingLatency, BigInteger consensusResponseLatency) {
        final Function function = new Function(
                FUNC_UPLOADINDICATORS, 
                Arrays.<Type>asList(new Uint256(dataQuality),
                new Uint256(consensusEnthusiasm),
                new Uint256(consensusSuccessRate),
                new Uint256(consensusProccesingLatency),
                new Uint256(consensusResponseLatency)),
                Collections.<TypeReference<?>>emptyList(), 0);
        return createSignedTransaction(function);
    }

    public Tuple5<BigInteger, BigInteger, BigInteger, BigInteger, BigInteger> getUploadIndicatorsInput(
            TransactionReceipt transactionReceipt) {
        String data = transactionReceipt.getInput().substring(10);
        final Function function = new Function(FUNC_UPLOADINDICATORS, 
                Arrays.<Type>asList(), 
                Arrays.<TypeReference<?>>asList(new TypeReference<Uint256>() {}, new TypeReference<Uint256>() {}, new TypeReference<Uint256>() {}, new TypeReference<Uint256>() {}, new TypeReference<Uint256>() {}));
        List<Type> results = this.functionReturnDecoder.decode(data, function.getOutputParameters());
        return new Tuple5<BigInteger, BigInteger, BigInteger, BigInteger, BigInteger>(

                (BigInteger) results.get(0).getValue(), 
                (BigInteger) results.get(1).getValue(), 
                (BigInteger) results.get(2).getValue(), 
                (BigInteger) results.get(3).getValue(), 
                (BigInteger) results.get(4).getValue()
                );
    }

    public static GaStateManagement load(String contractAddress, Client client,
            CryptoKeyPair credential) {
        return new GaStateManagement(contractAddress, client, credential);
    }

    public static GaStateManagement deploy(Client client, CryptoKeyPair credential) throws
            ContractException {
        return deploy(GaStateManagement.class, client, credential, getBinary(client.getCryptoSuite()), getABI(), null, null);
    }

    public static class BasicInfo extends StaticStruct {
        public BigInteger mem;

        public BigInteger cpu;

        public BigInteger sensor;

        public BigInteger bandWidth;

        public BasicInfo(Uint256 mem, Uint256 cpu, Uint256 sensor,
                Uint256 bandWidth) {
            super(mem,cpu,sensor,bandWidth);
            this.mem = mem.getValue();
            this.cpu = cpu.getValue();
            this.sensor = sensor.getValue();
            this.bandWidth = bandWidth.getValue();
        }

        public BasicInfo(BigInteger mem, BigInteger cpu, BigInteger sensor,
                BigInteger bandWidth) {
            super(new Uint256(mem),new Uint256(cpu),new Uint256(sensor),new Uint256(bandWidth));
            this.mem = mem;
            this.cpu = cpu;
            this.sensor = sensor;
            this.bandWidth = bandWidth;
        }
    }

    public static class Indicators extends StaticStruct {
        public BigInteger dataQuality;

        public BigInteger consensusEnthusiasm;

        public BigInteger consensusSuccessRate;

        public BigInteger consensusProccesingLatency;

        public BigInteger consensusResponseLatency;

        public Indicators(Uint256 dataQuality, Uint256 consensusEnthusiasm,
                Uint256 consensusSuccessRate, Uint256 consensusProccesingLatency,
                Uint256 consensusResponseLatency) {
            super(dataQuality,consensusEnthusiasm,consensusSuccessRate,consensusProccesingLatency,consensusResponseLatency);
            this.dataQuality = dataQuality.getValue();
            this.consensusEnthusiasm = consensusEnthusiasm.getValue();
            this.consensusSuccessRate = consensusSuccessRate.getValue();
            this.consensusProccesingLatency = consensusProccesingLatency.getValue();
            this.consensusResponseLatency = consensusResponseLatency.getValue();
        }

        public Indicators(BigInteger dataQuality, BigInteger consensusEnthusiasm,
                BigInteger consensusSuccessRate, BigInteger consensusProccesingLatency,
                BigInteger consensusResponseLatency) {
            super(new Uint256(dataQuality),new Uint256(consensusEnthusiasm),new Uint256(consensusSuccessRate),new Uint256(consensusProccesingLatency),new Uint256(consensusResponseLatency));
            this.dataQuality = dataQuality;
            this.consensusEnthusiasm = consensusEnthusiasm;
            this.consensusSuccessRate = consensusSuccessRate;
            this.consensusProccesingLatency = consensusProccesingLatency;
            this.consensusResponseLatency = consensusResponseLatency;
        }
    }
}
