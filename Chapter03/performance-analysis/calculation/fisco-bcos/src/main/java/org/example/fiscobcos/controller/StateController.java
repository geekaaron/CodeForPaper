package org.example.fiscobcos.controller;

import org.example.fiscobcos.contract.java.RegistrationManagement;
import org.example.fiscobcos.contract.java.StateManagement;
import org.fisco.bcos.sdk.v3.BcosSDK;
import org.fisco.bcos.sdk.v3.client.Client;
import org.fisco.bcos.sdk.v3.codec.datatypes.generated.tuples.generated.Tuple2;
import org.fisco.bcos.sdk.v3.crypto.keypair.CryptoKeyPair;
import org.fisco.bcos.sdk.v3.model.TransactionReceipt;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.io.File;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.List;

import static org.example.fiscobcos.controller.BcosSDKController.configFile;

@RestController
@RequestMapping("/StateController")
public class StateController {
    private static final List<Client> clients = new ArrayList<>();
    private static final String rmscAddress = "0x8533004032a32bb3a734d3dee2d07a1525bb7989";
    private static final String smscAddress = "0x7c83b3048225ceb9a0c2b65b9026e2f7865be1f8";
    private static Client ca;
    private static CryptoKeyPair caKeyPair;
    private static final BigInteger ROLE_NONE      = BigInteger.ZERO;
    private static final BigInteger ROLE_REQUESTER = BigInteger.ONE;
    private static final BigInteger ROLE_PROVIDER  = BigInteger.valueOf(2);
    private static final BigInteger ROLE_TEE       = BigInteger.valueOf(3);

    static {
        try {
            String caAccountPath =
                    "E:\\projects\\SGF\\framework-test\\fisco-bcos\\ca\\0x80b955d23e9f3bbf28ca2711009fcddb27afd0d0.pem";

            BcosSDK sdk = BcosSDK.build(configFile);
            ca = sdk.getClient();
            ca.getCryptoSuite().loadAccount("pem", caAccountPath, null);
            caKeyPair = ca.getCryptoSuite().getCryptoKeyPair();

            System.out.println("CA loaded.");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @GetMapping("/loadPemAccounts")
    public String loadPemAccounts() {
        try {
            String accountFolderPath = "E:\\projects\\SGF\\framework-test\\fisco-bcos\\pems";

            BcosSDK sdk = BcosSDK.build(configFile);
            File[] files = new File(accountFolderPath).listFiles();

            for (File file : files) {
                Client client = sdk.getClient();
                client.getCryptoSuite().loadAccount("pem", file.getPath(), null);
                clients.add(client);
            }
            return "Loaded accounts: " + clients.size();
        } catch (Exception e) {
            e.printStackTrace();
            return "Load failed";
        }
    }

    @GetMapping("/registerNodes")
    public String registerNodes(int providerCount, int requesterCount, int startCount, int splitCount) throws Exception {
        RegistrationManagement rm = RegistrationManagement.load(rmscAddress, ca, caKeyPair);
        TransactionReceipt ret;
        for (int i = startCount; i < startCount + providerCount; i++) {
            Client c = clients.get(i);
            String addr = c.getCryptoSuite().getCryptoKeyPair().getAddress();
            ret = rm.registerNode(addr, ROLE_PROVIDER);
        }

        for (int i = startCount; i < startCount + requesterCount; i++) {
            Client c = clients.get(splitCount + i);
            String addr = c.getCryptoSuite().getCryptoKeyPair().getAddress();
            ret = rm.registerNode(addr, ROLE_REQUESTER);
        }

        return "Register providers: " + providerCount + ", requester: " + requesterCount;
    }

    @GetMapping("/unregisterNodes")
    public String unregisterNodes(int providerCount, int requesterCount, int startCount, int splitCount) throws Exception {

        RegistrationManagement rm = RegistrationManagement.load(rmscAddress, ca, caKeyPair);

        long start = System.currentTimeMillis();

        // ===== 注销 Provider =====
        for (int i = startCount; i < startCount + providerCount; i++) {
            Client c = clients.get(i);
            String addr = c.getCryptoSuite()
                    .getCryptoKeyPair().getAddress();

            rm.unregisterNode(addr);
        }

        // ===== 注销 Requester =====
        for (int i = startCount; i < startCount + requesterCount; i++) {
            Client c = clients.get(splitCount + i);
            String addr = c.getCryptoSuite()
                    .getCryptoKeyPair().getAddress();

            rm.unregisterNode(addr);
        }

        long end = System.currentTimeMillis();

        return String.format(
                "Unregister Provider=%d, Requester=%d, cost=%d ms",
                providerCount,
                requesterCount,
                (end - start)
        );
    }

    @GetMapping("/uploadRecords")
    public String uploadRecords(int providerCount, int requesterCount, int startCount, int splitCount) throws Exception {
        long start = System.currentTimeMillis();

        // Provider
        for (int i = startCount; i < startCount + providerCount; i++) {
            Client p = clients.get(i);
            StateManagement sm = StateManagement.load(smscAddress, p, p.getCryptoSuite().getCryptoKeyPair());
            TransactionReceipt ret = sm.recordProviderConsensus(true);
            sm.recordProviderDelivery(true);
            sm.recordProviderDataQuality(new BigInteger(String.valueOf(80 + (i % 20))));
        }

        // Requester
        for (int i = startCount; i < startCount + requesterCount; i++) {
            Client r = clients.get(splitCount + i);
            StateManagement sm = StateManagement.load(smscAddress, r, r.getCryptoSuite().getCryptoKeyPair());
            TransactionReceipt ret = sm.recordRequesterConsensus(true);
            sm.recordRequesterPayment(true);
        }

        long end = System.currentTimeMillis();
        return "Upload records cost " + (end - start) + " ms";
    }

    @GetMapping("/getNodeCounts")
    public String getNodeCounts() throws Exception {

        StateManagement sm = StateManagement.load(smscAddress, ca, caKeyPair);

        Tuple2<List<String>, List<StateManagement.ProviderStats>> providerRes = sm.getAllProviderStats();
        Tuple2<List<String>, List<StateManagement.RequesterStats>> requesterRes = sm.getAllRequesterStats();

        int providerCount  = providerRes.getValue1().size();
        int requesterCount = requesterRes.getValue1().size();

        return String.format(
                "Providers=%d | Requesters=%d | Total=%d",
                providerCount,
                requesterCount,
                providerCount + requesterCount
        );
    }

    @GetMapping("/calcReputation")
    public String calcReputation(int nodeCount) throws Exception {
        int half = nodeCount / 2;

        StateManagement sm = StateManagement.load(smscAddress, ca, caKeyPair);

        long t1 = System.nanoTime();

        Tuple2<List<String>, List<StateManagement.ProviderStats>> providerRes = sm.getAllProviderStats();
        Tuple2<List<String>, List<StateManagement.RequesterStats>> requesterRes = sm.getAllRequesterStats();

        long t2 = System.nanoTime();

        List<StateManagement.ProviderStats> allProviders = providerRes.getValue2();
        List<StateManagement.RequesterStats> allRequesters = requesterRes.getValue2();

        int pN = Math.min(half, allProviders.size());
        int rN = Math.min(half, allRequesters.size());

        List<StateManagement.ProviderStats> pStats = allProviders.subList(0, pN);
        List<StateManagement.RequesterStats> rStats = allRequesters.subList(0, rN);

        final double alpha1DP = 0.4, alpha2DP = 0.3, alpha3DP = 0.3;
        final double beta1DP  = 0.4, beta2DP  = 0.3, beta3DP  = 0.3;

        final double alpha1DR = 0.5, alpha2DR = 0.5;
        final double beta1DR  = 0.5, beta2DR  = 0.5;

        final double lambda = 0.5;


        double[] PCSR = new double[pN];
        double[] OTDR = new double[pN];
        double[] ADQ  = new double[pN];

        long[] PCC = new long[pN];
        long[] DST = new long[pN];

        long maxPCC = 0;
        long maxDST = 0;
        double maxADQ = 0.0;

        for (int i = 0; i < pN; i++) {
            StateManagement.ProviderStats s = pStats.get(i);

            long pcc  = s.consensusCount.longValue();
            long pcsc = s.consensusSuccessCount.longValue();

            long dst  = s.deliveryCount.longValue();
            long otdc = s.timelyDeliveryCount.longValue();

            double adq = safeDiv(s.totalDataQuality.doubleValue(), s.dataProductCount.doubleValue());

            PCC[i] = pcc;
            DST[i] = dst;

            PCSR[i] = ratio(pcsc, pcc);
            OTDR[i] = ratio(otdc, dst);
            ADQ[i]  = adq;

            if (pcc > maxPCC) maxPCC = pcc;
            if (dst > maxDST) maxDST = dst;
            if (adq > maxADQ) maxADQ = adq;
        }

        double[] Rp = new double[pN];

        for (int i = 0; i < pN; i++) {
            double Rl = alpha1DP * PCSR[i] + alpha2DP * OTDR[i] + alpha3DP * ADQ[i];

            double GPCSR = safeDiv((double) PCC[i], (double) maxPCC) * PCSR[i];
            double GOTDR = safeDiv((double) DST[i], (double) maxDST) * OTDR[i];
            double GADQ  = safeDiv(ADQ[i], maxADQ) * ADQ[i];
            double Rg = beta1DP * GPCSR + beta2DP * GOTDR + beta3DP * GADQ;

            Rp[i] = lambda * Rl + (1.0 - lambda) * Rg;
        }

        double[] RCSR = new double[rN];
        double[] OTPR = new double[rN];

        long[] RCC = new long[rN];
        long[] DRT = new long[rN];

        long maxRCC = 0;
        long maxDRT = 0;

        for (int i = 0; i < rN; i++) {
            StateManagement.RequesterStats s = rStats.get(i);

            long rcc  = s.consensusCount.longValue();
            long rcsc = s.consensusSuccessCount.longValue();

            long drt  = s.paymentCount.longValue();
            long otpc = s.timelyPaymentCount.longValue();

            RCC[i] = rcc;
            DRT[i] = drt;

            RCSR[i] = ratio(rcsc, rcc);
            OTPR[i] = ratio(otpc, drt);

            if (rcc > maxRCC) maxRCC = rcc;
            if (drt > maxDRT) maxDRT = drt;
        }

        double[] Rr = new double[rN];

        for (int i = 0; i < rN; i++) {
            double Rl = alpha1DR * RCSR[i] + alpha2DR * OTPR[i];

            double GRCSR = safeDiv((double) RCC[i], (double) maxRCC) * RCSR[i];
            double GOTPR = safeDiv((double) DRT[i], (double) maxDRT) * OTPR[i];
            double Rg = beta1DR * GRCSR + beta2DR * GOTPR;

            Rr[i] = lambda * Rl + (1.0 - lambda) * Rg;
        }

        long t3 = System.nanoTime();

        double chainReadMs = (t2 - t1) / 1_000_000.0;
        double calcMs      = (t3 - t2) / 1_000_000.0;
        double totalMs     = (t3 - t1) / 1_000_000.0;

        return String.format(
                "Nodes=%d | Provider=%d | Requester=%d | ChainRead=%.3f ms | Calc=%.3f ms | Total=%.3f ms",
                nodeCount, pN, rN, chainReadMs, calcMs, totalMs
        );
    }

    private static double ratio(long num, long den) {
        return den == 0 ? 0.0 : (double) num / (double) den;
    }

    private double safeDiv(double a, double b) {
        return b == 0 ? 0 : a / b;
    }
}
