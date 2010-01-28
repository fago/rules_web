<?php
// $Id$

/**
 * @file Rules Remote Sites - Include file.
 */

/**
 * Class representing remote sites.
 */
class RulesWebRemote extends EntityDB {
  
  public $settings = array();
  public $name;
  public $label;
  public $url;
  
  protected $endpoint = NULL;
  
  function __construct($values = array(), $entity_type = 'rules_web_remote') {
    parent::__construct($values, $entity_type);
  }
  
  public function access($account = NULL) {
    return user_access('interact with remote ' . $this->name, $account);
  }
  
  /**
   * @return RulesWebRemoteEndpointInterface
   */
  protected function endpoint() {
    if (!isset($this->endpoint)) {
      $types = rules_web_remote_get_types();
      $this->endpoint = new $types[$this->type]['class']($this);
    }
    return $this->endpoint;
  }
  
  public function load($ids) {
    return $this->endpoint()->load($ids);
  }
  
  public function events() {
    return $this->endpoint()->events();
  }
  
  public function dataTypes() {
    return $this->endpoint()->dataTypes();
  }
  
  public function actions() {
    return $this->endpoint()->actions();
  }
  
  public function conditions() {
    return $this->endpoint()->conditions();
  }
}

/**
 * Interface for remote endpoints.
 */
interface RulesWebRemoteEndpointInterface {
  
  public function load($ids);
  
  public function events();
  
  public function dataTypes();
  
  public function actions();
  
  public function conditions();
}

/**
 * A remote endpoint types for rules web hooks.
 */
class RulesWebRemoteEndpointWebHooks implements RulesWebRemoteEndpointInterface {
  
  /**
   * @var RulesWebRemote
   */
  protected $remote, $url;
  
  /**
   * @var RestClient
   */
  protected  $client;
  
  public function __construct($remote, $base_path = 'rules_web') {
    $this->remote = $remote;
    $this->url = $remote->url . '/' . $base_path . '/';
  }
  
  protected function client() {
    if (!isset($this->client)) {
      $this->client = new RestClient(NULL, new RestClientBaseFormatter(RestClientBaseFormatter::FORMAT_JSON));
      // Pass through additional curl options.
      if (!empty($this->remote->settings['curl options'])) {
        $this->client->curlOpts = $this->remote->settings['curl options'];
      }
    }
    return $this->client;
  }
  
  public function load($ids) {
    
  }
  
  public function events() {
    
  }
  
  public function dataTypes() {
    return $this->client()->get($this->url . 'entity_metadata.json', array());
  }
  
  public function actions() {
    
  }
  
  public function conditions() {
    
  }
}

/**
 * A controller for loading remote data.
 */
class RulesWebRemoteEntityController extends DrupalDefaultEntityController {
  
  protected $remote;
  
  function __construct($entityType) {
    parent::__contruct($entityType);
    $this->remote = entity_load('rules_web_remote', array($this->entityInfo['rules web remote']));
  }
  
  
  /**
   * Override load to fetch the data from the remote site. For now we don't
   * support using $conditions or revisions.
   */
  public function load($ids = array(), $conditions = array()) {
    $this->ids = $ids;
    $this->conditions = $conditions;

    $entities = array();

    // Create a new variable which is either a prepared version of the $ids
    // array for later comparison with the entity cache, or FALSE if no $ids
    // were passed. The $ids array is reduced as items are loaded from cache,
    // and we need to know if it's empty for this reason to avoid querying the
    // database when all requested entities are loaded from cache.
    $passed_ids = !empty($this->ids) ? array_flip($this->ids) : FALSE;
    // Try to load entities from the static cache, if the entity type supports
    // static caching.
    if ($this->cache) {
      $entities += $this->cacheGet($this->ids, $this->conditions);
      // If any entities were loaded, remove them from the ids still to load.
      if ($passed_ids) {
        $this->ids = array_keys(array_diff_key($passed_ids, $entities));
      }
    }

    // Load any remaining entities from the remote site.
    if ($this->ids === FALSE || $this->ids) {
      $queried_entities = $this->remote->load($this->ids);
    }

    // Pass all entities loaded from the database through $this->attachLoad(),
    // which attaches fields (if supported by the entity type) and calls the
    // entity type specific load callback, for example hook_node_load().
    if (!empty($queried_entities)) {
      $this->attachLoad($queried_entities);
      $entities += $queried_entities;
    }

    if ($this->cache) {
      // Add entities to the cache if we are not loading a revision.
      if (!empty($queried_entities) && !$this->revisionId) {
        $this->cacheSet($queried_entities);
      }
    }

    // Ensure that the returned array is ordered the same as the original
    // $ids array if this was passed in and remove any invalid ids.
    if ($passed_ids) {
      // Remove any invalid ids from the array.
      $passed_ids = array_intersect_key($passed_ids, $entities);
      foreach ($entities as $entity) {
        $passed_ids[$entity->{$this->idKey}] = $entity;
      }
      $entities = $passed_ids;
    }

    return $entities;
  }
}

