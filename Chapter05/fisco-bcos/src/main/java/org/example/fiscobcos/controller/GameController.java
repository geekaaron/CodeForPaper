package org.example.fiscobcos.controller;

import org.example.fiscobcos.contract.java.*;
import org.example.fiscobcos.entity.UploadGameResultRequest;
import org.fisco.bcos.sdk.v3.BcosSDK;
import org.fisco.bcos.sdk.v3.client.Client;
import org.fisco.bcos.sdk.v3.crypto.keypair.CryptoKeyPair;
import org.json.JSONArray;
import org.json.JSONObject;
import org.springframework.web.bind.annotation.*;

import java.io.File;
import java.util.*;

import static org.example.fiscobcos.controller.BcosSDKController.client;
import static org.example.fiscobcos.controller.BcosSDKController.configFile;
import static org.fisco.bcos.sdk.v3.utils.ByteUtils.hexStringToBytes;

@RestController
@RequestMapping("/GameController")
public class GameController {
    private static List<Client> clients = new ArrayList<>();
    private static List<Client> providers = new ArrayList<>();
    private static List<Client> requesters = new  ArrayList<>();
    private static List<Client> consensus = new  ArrayList<>();
    private static List<Client> tees = new  ArrayList<>();

    private static String gmscAddress = "0x6849f21d1e455e9f0712b1e99fa4fcd23758e8f1"
    public static CryptoKeyPair keyPair = client.getCryptoSuite().getCryptoKeyPair();

    @GetMapping("/loadPemAccounts")
    public void loadPemAccounts() {
        try {
            String accountFolderPath = "/home/q/projects/SGF/framework-test/fisco-bcos/pems";
            BcosSDK sdk = BcosSDK.build(configFile);
            File folder = new File(accountFolderPath);
            File[] files =  folder.listFiles();
            for (File file : files) {
                Client client = sdk.getClient();
                client.getCryptoSuite().loadAccount("pem", file.getPath(), null);
                clients.add(client);
            }
            System.out.println("Load complete.");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @GetMapping("/allocateNodeAccounts")
    public void allocateNodeAccounts(@RequestParam int nProviders, @RequestParam int nRequesters,
                                     @RequestParam int nConsensus, @RequestParam int nTees) {
        int totalRequired = nProviders + nRequesters + nTees;
        if (clients.size() < totalRequired) {
            System.out.println("Not enough clients to allocate roles.");
            return;
        }

        providers.clear();
        requesters.clear();
        consensus.clear();
        tees.clear();

        List<Client> shuffled = new ArrayList<>(clients);
        Collections.shuffle(shuffled);

        Set<Client> used = new HashSet<>();
        for (int i = 0; i < nProviders; i++) {
            Client c = shuffled.get(i);
            providers.add(c);
            used.add(c);
        }

        int rCount = 0, idx = nProviders;
        while (rCount < nRequesters && idx < shuffled.size()) {
            Client c = shuffled.get(idx++);
            if (!used.contains(c)) {
                requesters.add(c);
                used.add(c);
                rCount++;
            }
        }

        List<Client> consensusCandidates = new ArrayList<>();
        consensusCandidates.addAll(providers);
        consensusCandidates.addAll(requesters);
        Collections.shuffle(consensusCandidates);
        for (int i = 0; i < Math.min(nConsensus, consensusCandidates.size()); i++) {
            consensus.add(consensusCandidates.get(i));
        }

        int tCount = 0;
        while (tCount < nTees && idx < shuffled.size()) {
            Client c = shuffled.get(idx++);
            if (!used.contains(c)) {
                tees.add(c);
                used.add(c);
                tCount++;
            }
        }
        System.out.println("Allocate complete.");
    }

    @GetMapping("/uploadParams")
    public void uploadParams(@RequestParam String paramsPath, @RequestParam String teeInfosPath) {
        try {
            String paramsJson = new String(java.nio.file.Files.readAllBytes(new File(paramsPath).toPath()));
            String teeJson = new String(java.nio.file.Files.readAllBytes(new File(teeInfosPath).toPath()));

            JSONObject root = new JSONObject(paramsJson);
            JSONObject providersObj = root.getJSONObject("providers");
            JSONArray prices = providersObj.getJSONArray("prices");
            JSONArray amounts = providersObj.getJSONArray("amounts");
            JSONArray reputations = providersObj.getJSONArray("reputations");
            JSONArray costs = providersObj.getJSONArray("costs");

            JSONObject requestersObj = root.getJSONObject("requesters");
            JSONArray lambda1s = requestersObj.getJSONArray("lambda1s");
            JSONArray lambda2s = requestersObj.getJSONArray("lambda2s");
            JSONArray sensitivities = requestersObj.getJSONArray("sensitivities");

            for (int i = 0; i < providers.size() && i < prices.length(); i++) {
                Client client = providers.get(i);
                CryptoKeyPair kp = client.getCryptoSuite().getCryptoKeyPair();
                GameManagement instance = GameManagement.load(gmscAddress, client, kp);

                instance.uploadProvider(
                        String.valueOf(prices.getDouble(i)),
                        String.valueOf(amounts.getInt(i)),
                        String.valueOf(reputations.getDouble(i)),
                        String.valueOf(costs.getDouble(i))
                );
            }

            for (int i = 0; i < requesters.size() && i < lambda1s.length(); i++) {
                Client client = requesters.get(i);
                CryptoKeyPair kp = client.getCryptoSuite().getCryptoKeyPair();
                GameManagement instance = GameManagement.load(gmscAddress, client, kp);

                instance.uploadRequester(
                        String.valueOf(lambda1s.getInt(i)),
                        String.valueOf(lambda2s.getDouble(i)),
                        String.valueOf(sensitivities.getDouble(i))
                );
            }

            JSONObject teeRoot = new JSONObject(teeJson);
            List<String> electedTeeCandidates = new ArrayList<>();
            for (int i = 0; i < tees.size(); i++) {
                String teeKey = "Tee" + (i + 1);
                if (!teeRoot.has(teeKey)) break;

                JSONObject teeInfo = teeRoot.getJSONObject(teeKey);
                Client client = tees.get(i);
                CryptoKeyPair kp = client.getCryptoSuite().getCryptoKeyPair();
                GameManagement instance = GameManagement.load(gmscAddress, client, kp);

                instance.uploadTeeInfo(
                        hexStringToBytes(teeInfo.getString("raPublicKey")),
                        hexStringToBytes(teeInfo.getString("ecdsaPublicKey")),
                        hexStringToBytes(teeInfo.getString("taShdrHash")),
                        hexStringToBytes(teeInfo.getString("taMemHash")),
                        hexStringToBytes(teeInfo.getString("teeMemHash"))
                );

                electedTeeCandidates.add(kp.getAddress());
            }

            if (!electedTeeCandidates.isEmpty()) {
                Random random = new Random();
                int idx = random.nextInt(electedTeeCandidates.size());
                String selectedAddress = electedTeeCandidates.get(idx);

                Client caller = tees.get(0);
                CryptoKeyPair kp = caller.getCryptoSuite().getCryptoKeyPair();
                GameManagement controller = GameManagement.load(gmscAddress, caller, kp);
                controller.electTee(selectedAddress);

                System.out.println("TEE elected: " + selectedAddress);
            }

            for (Client consClient : consensus) {
                CryptoKeyPair kp = consClient.getCryptoSuite().getCryptoKeyPair();
                GameManagement controller = GameManagement.load(gmscAddress, client, kp);
                controller.electConsensus(kp.getAddress());

                System.out.println("Consensus node elected: " + kp.getAddress());
            }

            System.out.println("Upload complete.");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @GetMapping("/getGameParams")
    public Map<String, Object> getGameParams() {
        Map<String, Object> result = new HashMap<>();
        try {
            GameManagement instance = GameManagement.load(gmscAddress, client, keyPair);

            List<GameManagement.ProviderInfo> providersInfo = instance.getProvidersInfo();
            List<Map<String, String>> providersArray = new ArrayList<>();
            for (GameManagement.ProviderInfo p : providersInfo) {
                Map<String, String> item = new HashMap<>();
                item.put("price", p.price);
                item.put("amount", p.amount);
                item.put("reputation", p.reputation);
                item.put("cost", p.cost);
                providersArray.add(item);
            }

            List<GameManagement.RequesterInfo> requestersInfo = instance.getRequestersInfo();
            List<Map<String, String>> requestersArray = new ArrayList<>();
            for (GameManagement.RequesterInfo r : requestersInfo) {
                Map<String, String> item = new HashMap<>();
                item.put("lambda1", r.lambda1);
                item.put("lambda2", r.lambda2);
                item.put("sensitivity", r.sensitivity);
                requestersArray.add(item);
            }

            result.put("providers", providersArray);
            result.put("requesters", requestersArray);
        } catch (Exception e) {
            e.printStackTrace();
            result.put("error", e.getMessage());
        }
        return result;
    }

    public static String bytesToHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }

    @GetMapping("/getElectedTeeInfo")
    public Map<String, String> getElectedTeeInfo() {
        Map<String, String> result = new HashMap<>();
        try {
            GameManagement instance = GameManagement.load(gmscAddress, client, keyPair);
            GameManagement.TeeInfos teeInfo = instance.getElectedTeeInfo();

            result.put("raPublicKey", bytesToHex(teeInfo.raPublicKey));
            result.put("ecdsaPublicKey", bytesToHex(teeInfo.ecdsaPublicKey));
            result.put("taShdrHash", bytesToHex(teeInfo.taShdrHash));
            result.put("taMemHash", bytesToHex(teeInfo.taMemHash));
            result.put("teeMemHash", bytesToHex(teeInfo.teeMemHash));
        } catch (Exception e) {
            e.printStackTrace();
            result.put("error", e.getMessage());
        }
        return result;
    }

    @GetMapping("/uploadConsensusResult")
    public void uploadConsensusResult(@RequestParam String hashHex,
                                      @RequestParam String nonceHex,
                                      @RequestParam String signatureHex,
                                      @RequestParam int nodeIndex) {
        try {
            if (nodeIndex >= consensus.size()) {
                System.out.println("Invalid consensus node index.");
                return;
            }

            Client node = consensus.get(nodeIndex);
            CryptoKeyPair kp = node.getCryptoSuite().getCryptoKeyPair();
            GameManagement instance = GameManagement.load(gmscAddress, node, kp);

            instance.uploadConsensusResult(
                    hexStringToBytes(hashHex),
                    hexStringToBytes(nonceHex),
                    hexStringToBytes(signatureHex)
            );

            System.out.println("Consensus result uploaded by node " + kp.getAddress());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @GetMapping("/uploadGameParamsInfo")
    public void uploadGameParamsInfo(@RequestParam String hashHex,
                                     @RequestParam String nonceHex,
                                     @RequestParam String signatureHex) {
        try {
            GameManagement controller = GameManagement.load(gmscAddress, client, keyPair);
            String electedTeeAddr = controller.getElectedTeeAddress();

            Client electedTee = null;
            for (Client tee : tees) {
                String addr = tee.getCryptoSuite().getCryptoKeyPair().getAddress();
                if (addr.equalsIgnoreCase(electedTeeAddr)) {
                    electedTee = tee;
                    break;
                }
            }

            if (electedTee == null) {
                System.out.println("Elected TEE address not found in local tees.");
                return;
            }

            CryptoKeyPair kp = electedTee.getCryptoSuite().getCryptoKeyPair();
            GameManagement instance = GameManagement.load(gmscAddress, electedTee, kp);

            instance.uploadConsensusResult(
                    hexStringToBytes(hashHex),
                    hexStringToBytes(nonceHex),
                    hexStringToBytes(signatureHex)
            );

            System.out.println("Game params info uploaded by elected TEE: " + electedTeeAddr);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @PostMapping("/uploadGameResult")
    public void uploadGameResult(@RequestBody UploadGameResultRequest request) {
        try {
            GameManagement controller = GameManagement.load(gmscAddress, client, keyPair);
            String electedTeeAddr = controller.getElectedTeeAddress();

            Client electedTee = null;
            for (Client tee : tees) {
                String addr = tee.getCryptoSuite().getCryptoKeyPair().getAddress();
                if (addr.equalsIgnoreCase(electedTeeAddr)) {
                    electedTee = tee;
                    break;
                }
            }

            if (electedTee == null) {
                System.out.println("Elected TEE address not found in local tees.");
                return;
            }

            CryptoKeyPair kp = electedTee.getCryptoSuite().getCryptoKeyPair();
            GameManagement instance = GameManagement.load(gmscAddress, electedTee, kp);

            instance.uploadGameResult(
                    request.getOptimalPrices(),
                    hexStringToBytes(request.getNonceHex()),
                    hexStringToBytes(request.getSignatureHex())
            );

            System.out.println("Game result uploaded by elected TEE: " + electedTeeAddr);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @GetMapping("/getConsensusResult")
    public Map<String, String> getConsensusResult(@RequestParam String address) {
        Map<String, String> result = new HashMap<>();
        try {
            GameManagement instance = GameManagement.load(gmscAddress, client, keyPair);
            GameManagement.ConsensusResult consensus = instance.getConsensusResult(address);

            result.put("hash", bytesToHex(consensus.hash));
            result.put("nonce", bytesToHex(consensus.nonce));
            result.put("signature", bytesToHex(consensus.signature));
        } catch (Exception e) {
            e.printStackTrace();
            result.put("error", e.getMessage());
        }
        return result;
    }

    @GetMapping("/getGameResult")
    public Map<String, String> getGameResult(@RequestParam String address) {
        Map<String, String> result = new HashMap<>();
        try {
            GameManagement instance = GameManagement.load(gmscAddress, client, keyPair);
            GameManagement.GameResult game = instance.getGameResult(address);

            result.put("optimalPrices", game.optimalPrices);
            result.put("nonce", bytesToHex(game.nonce));
            result.put("signature", bytesToHex(game.signature));
        } catch (Exception e) {
            e.printStackTrace();
            result.put("error", e.getMessage());
        }
        return result;
    }

    @GetMapping("/clear")
    public void clear() {
        try {
            GameManagement instance = GameManagement.load(gmscAddress, client, keyPair);
            instance.clear();
            System.out.println("GameManagement state cleared.");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
