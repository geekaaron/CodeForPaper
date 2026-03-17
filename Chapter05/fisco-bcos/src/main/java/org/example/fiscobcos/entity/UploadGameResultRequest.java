package org.example.fiscobcos.entity;

import lombok.Data;

@Data
public class UploadGameResultRequest {
    private String optimalPrices;
    private String nonceHex;
    private String signatureHex;
}