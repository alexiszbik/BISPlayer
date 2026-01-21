//
//  ViewController.swift
//  launchBIS
//
//  Created by Alexis ZBIK on 21/01/2026.
//

import Cocoa

class ViewController: NSViewController {

    let appBundleID = "com.yourcompany.BISPlayer" // Remplace par le bundle ID de l'app à surveiller
    
    
    func isAppRunning(bundleID: String) -> Bool {
        return NSWorkspace.shared.runningApplications.contains { $0.bundleIdentifier == bundleID }
    }
    
    
    
    func launchApp(bundleID: String) {
        guard let appURL = NSWorkspace.shared.urlForApplication(withBundleIdentifier: bundleID) else {
            print("Impossible de trouver l'application \(bundleID)")
            return
        }
        do {
            try NSWorkspace.shared.launchApplication(at: appURL,
                                                     options: [.default],
                                                     configuration: [:])
            print("\(bundleID) lancé !")
        } catch {
            print("Erreur au lancement : \(error)")
        }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // Do any additional setup after loading the view.
        
        // Timer qui vérifie toutes les secondes
        Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { _ in
            if !self.isAppRunning(bundleID: self.appBundleID) {
                print("L'application n'est pas lancée, on la démarre…")
                self.launchApp(bundleID: self.appBundleID)
            }
        }
        
        // Do any additional setup after loading the view.
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }


}
