package org.example.fiscobcos.controller;

import org.fisco.bcos.sdk.v3.BcosSDK;
import org.fisco.bcos.sdk.v3.client.Client;
import org.fisco.bcos.sdk.v3.client.protocol.response.BcosBlock;
import org.fisco.bcos.sdk.v3.client.protocol.response.BlockNumber;
import org.springframework.web.bind.annotation.*;

import java.math.BigInteger;

@RestController
@RequestMapping("/BcosSDKController")
public class BcosSDKController {
    public static String configFile = "./conf/config.toml";
    public static Client client = BcosSDK.build(configFile).getClient("group0");

    @GetMapping("/getBlockNumber")
    public BlockNumber getBlockNumber() {
        return client.getBlockNumber();
    }

    @GetMapping("/getBlockByNumber")
    public BcosBlock getBlockByNumber(@RequestParam BigInteger number) {
        return client.getBlockByNumber(number, true, true);
    }
}
