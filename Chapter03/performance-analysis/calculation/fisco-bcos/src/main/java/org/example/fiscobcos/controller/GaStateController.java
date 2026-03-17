package org.example.fiscobcos.controller;

import org.example.fiscobcos.contract.java.GaStateManagement;
import org.fisco.bcos.sdk.v3.BcosSDK;
import org.fisco.bcos.sdk.v3.client.Client;
import org.fisco.bcos.sdk.v3.codec.datatypes.generated.tuples.generated.Tuple2;
import org.fisco.bcos.sdk.v3.crypto.keypair.CryptoKeyPair;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.io.File;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.List;

import static org.example.fiscobcos.controller.BcosSDKController.configFile;

@RestController
@RequestMapping("/GaStateController")
public class GaStateController {
    private static final List<Client> clients = new ArrayList<>();
    private static final String gsmscAddress = "0x67372101230ff7ea4f8d7fac84d8755683a13d2f";
    private static Client ca;
    private static CryptoKeyPair caKeyPair;

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

    @GetMapping("/uploadRecords")
    public String uploadRecords(int nodeCount, int startIndex) throws Exception {
        long start = System.currentTimeMillis();

        for (int i = startIndex; i < startIndex + nodeCount; i++) {
            Client c = clients.get(i);
            CryptoKeyPair kp = c.getCryptoSuite().getCryptoKeyPair();

            GaStateManagement gsm = GaStateManagement.load(gsmscAddress, c, kp);

            BigInteger mem = BigInteger.valueOf(50 + (i % 50));       // 50~99
            BigInteger cpu = BigInteger.valueOf(40 + (i % 60));       // 40~99
            BigInteger sensor = BigInteger.valueOf(30 + (i % 70));    // 30~99
            BigInteger bw = BigInteger.valueOf(20 + (i % 80));        // 20~99

            gsm.uploadBasic(mem, cpu, sensor, bw);

            BigInteger psi = BigInteger.valueOf(60 + (i % 40));     // 60~99
            BigInteger rho = BigInteger.valueOf(50 + (i % 50));     // 50~99
            BigInteger gamma = BigInteger.valueOf(70 + (i % 30));   // 70~99
            BigInteger tauP = BigInteger.valueOf(10 + (i % 90));
            BigInteger tauR = BigInteger.valueOf(15 + (i % 85));

            gsm.uploadIndicators(psi, rho, gamma, tauP, tauR);
            gsm.uploadFinalReputation(new BigInteger("0"));
        }

        long endMs = System.currentTimeMillis();
        return String.format("UploadRecords nodes=%d cost=%d ms", nodeCount, (endMs - start));
    }

    @GetMapping("/getNodeCounts")
    public String getNodeCounts() throws Exception {
        GaStateManagement gsm = GaStateManagement.load(gsmscAddress, ca, caKeyPair);

        Tuple2<List<String>, List<GaStateManagement.BasicInfo>> basicRes = gsm.getAllBasic();
        Tuple2<List<String>, List<GaStateManagement.Indicators>> indRes = gsm.getAllIndicators();
        Tuple2<List<String>, List<BigInteger>> rtRes = gsm.getAllFinalReputation();

        int basicN = basicRes.getValue1().size();
        int indN = indRes.getValue1().size();
        int rtN = rtRes.getValue1().size();

        return String.format(
                "Basic=%d | Indicators=%d | FinalRt=%d",
                basicN, indN, rtN
        );
    }

    @GetMapping("/calcReputation")
    public String calcReputation(int nodeCount) throws Exception {
        if (nodeCount <= 0) return "nodeCount must be > 0";

        GaStateManagement gsm = GaStateManagement.load(gsmscAddress, ca, caKeyPair);

        long t1 = System.nanoTime();

        Tuple2<List<String>, List<GaStateManagement.BasicInfo>> basicRes = gsm.getAllBasic();
        Tuple2<List<String>, List<GaStateManagement.Indicators>> indRes = gsm.getAllIndicators();
        Tuple2<List<String>, List<BigInteger>> rtRes = gsm.getAllFinalReputation();

        long t2 = System.nanoTime();

        List<GaStateManagement.BasicInfo> allBasic = basicRes.getValue2();
        List<GaStateManagement.Indicators> allIndicators = indRes.getValue2();
        List<BigInteger> allReputation = rtRes.getValue2();

        List<GaStateManagement.BasicInfo> basicInfos = allBasic.subList(0, nodeCount);
        List<GaStateManagement.Indicators> indicators = allIndicators.subList(0, nodeCount);
        List<BigInteger> reputations = allReputation.subList(0, nodeCount);

        double[] Rd = new double[nodeCount];
        for (int i = 0; i < nodeCount; i++) {
            GaStateManagement.BasicInfo b = basicInfos.get(i);
            Rd[i] = b.mem.doubleValue()
                    + b.cpu.doubleValue()
                    + b.sensor.doubleValue()
                    + b.bandWidth.doubleValue();
            Rd[i] = sigmoid(Rd[i]);
        }

        double[] psi = new double[nodeCount];
        double[] rho = new double[nodeCount];
        double[] G = new double[nodeCount];
        double[] tauP = new double[nodeCount];
        double[] tauR = new double[nodeCount];

        double maxPsi = 0, maxRho = 0, maxG = 0;
        double minTauP = Double.MAX_VALUE, minTauR = Double.MAX_VALUE;
        double maxTauP = 0, maxTauR = 0;

        for (int i = 0; i < nodeCount; i++) {
            GaStateManagement.Indicators c = indicators.get(i);
            psi[i] = c.dataQuality.doubleValue();
            rho[i] = c.consensusEnthusiasm.doubleValue();
            G[i] = c.consensusSuccessRate.doubleValue();
            tauP[i] = c.consensusProccesingLatency.doubleValue();
            tauR[i] = c.consensusResponseLatency.doubleValue();

            maxPsi = Math.max(maxPsi, psi[i]);
            maxRho = Math.max(maxRho, rho[i]);
            maxG = Math.max(maxG, G[i]);

            minTauP = Math.min(minTauP, tauP[i]);
            minTauR = Math.min(minTauR, tauR[i]);
            maxTauP = Math.max(maxTauP, tauP[i]);
            maxTauR = Math.max(maxTauR, tauR[i]);
        }

        double[] Rc = new double[nodeCount];
        for (int i = 0; i < nodeCount; i++) {
            double nPsi = (maxPsi == 0.0) ? 0.0 : psi[i] / maxPsi;
            double nRho = (maxRho == 0.0) ? 0.0 : rho[i] / maxRho;
            double nG   = (maxG   == 0.0) ? 0.0 : G[i] / maxG;

            double nTauP = (maxTauP == minTauP) ? 0.0
                    : (maxTauP - tauP[i]) / (maxTauP - minTauP);

            double nTauR = (maxTauR == minTauR) ? 0.0
                    : (maxTauR - tauR[i]) / (maxTauR - minTauR);

            Rc[i] = (nPsi + nRho + nG + nTauP + nTauR) / 5.0;
        }

        double[] Rprime = new double[nodeCount];
        double[] Rfinal = new double[nodeCount];
        for (int i = 0; i < nodeCount; i++) {
            Rprime[i] = (Rd[i] + Rc[i]) / 2.0;
            Rfinal[i] = sigmoid(Rprime[i] - reputations.get(i).doubleValue());
        }

        long t3 = System.nanoTime();

        double chainReadMs = (t2 - t1) / 1_000_000.0;
        double calcMs = (t3 - t2) / 1_000_000.0;
        double totalMs = (t3 - t1) / 1_000_000.0;

        return String.format(
                "Nodes=%d | ChainRead=%.3f ms | Calc=%.3f ms | Total=%.3f ms",
                nodeCount, chainReadMs, calcMs, totalMs
        );
    }

    private static double sigmoid(double x) {
        return 1.0 / (1.0 + Math.exp(-x));
    }
}
